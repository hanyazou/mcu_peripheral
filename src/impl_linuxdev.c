/*
 * MIT License
 *
 * Copyright (c) 2024 hanyazou
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <linux/spi/spidev.h>

#include <mcu_peripheral/mcu_peripheral.h>
#include <mcu_peripheral/log.h>

/*
 * Implementation for linux device interfaces:
 *  - GPIO via sysfs (/sys/class/gpio)
 *  - I2C via /dev/i2c-N
 *  - SPI via /dev/spidevN.M
 *
 */

static int sysfs_gpio_export(int pin);
static int sysfs_gpio_unexport(int pin);
static int sysfs_gpio_set_dir(int pin, int is_output);
static int sysfs_gpio_write_value(int pin, int value);
static int sysfs_gpio_unexport(int pin);
static int sysfs_gpio_read_value(int pin);

/*=================================================================================================
 * GPIO API
 */

struct linuxdev_gpio_data {
    int dummy;
};

mcupr_result_t mcupr_gpio_chip_create(mcupr_gpio_chip_t **chipp, mcupr_gpio_chip_params_t *params)
{
    mcupr_gpio_chip_t *chip;
    mcupr_result_t result = MCUPR_RES_UNKNOWN;

    /* Allocate chip object */
    chip = calloc(1, sizeof(mcupr_gpio_chip_t) + sizeof(struct linuxdev_gpio_data));
    if (chip == NULL) {
        MCUPR_ERR("%s: memory allocation failed", __func__);
        return MCUPR_RES_NOMEM;
    }

    struct linuxdev_gpio_data *priv = (struct linuxdev_gpio_data *)&chip[1];
    chip->data = priv;

    *chipp = chip;

    return MCUPR_RES_OK;
}

mcupr_result_t mcupr_gpio_open(mcupr_gpio_chip_t *chip, int pin, mcupr_gpio_mode_t mode)
{
    (void)chip; // not used in sysfs example

    sysfs_gpio_export(pin);

    return sysfs_gpio_set_dir(pin, mode == MCUPR_GPIO_MODE_OUTPUT);
}

/* Write value (0 or 1) */
void mcupr_gpio_write(mcupr_gpio_chip_t *chip, int pin, int value)
{
    (void)chip; // not used
    sysfs_gpio_write_value(pin, value);
}

/* Read the pin value (0 or 1, -1 on error) */
int mcupr_gpio_read(mcupr_gpio_chip_t *chip, int pin)
{
    (void)chip; // not used
    return sysfs_gpio_read_value(pin);
}

/* Optionally unexport the pin if desired. */
void mcupr_gpio_chip_release(mcupr_gpio_chip_t *chip)
{
    (void)chip; // not used
}

/*=================================================================================================
 * I2C API (via /dev/i2c-X)
 */

struct linuxdev_i2c_data {
    int busnum;
};

mcupr_result_t mcupr_i2c_bus_create(mcupr_i2c_bus_t **busp, const mcupr_i2c_bus_params_t *params)
{
    int i;
    mcupr_i2c_bus_t *bus;

    /* Allocate bus object */
    bus = calloc(1, sizeof(mcupr_i2c_bus_t) + sizeof(struct linuxdev_i2c_data));
    if (bus == NULL) {
        MCUPR_ERR("%s: memory allocation failed", __func__);
        return MCUPR_RES_NOMEM;
    }

    struct linuxdev_i2c_data *priv = (struct linuxdev_i2c_data *)&bus[1];
    bus->data = priv;
    priv->busnum = params->busnum;
    if (priv->busnum == MCUPR_UNSPECIFIED) {
        priv->busnum = 0;
    }
    *busp = bus;

    return MCUPR_RES_OK;
}

void mcupr_i2c_bus_release(mcupr_i2c_bus_t *bus)
{
    if (bus == NULL || bus->data == NULL) {
        return;
    }
    struct linuxdev_i2c_data *priv = (struct linuxdev_i2c_data *)bus->data;

    memset(priv, 0, sizeof(*priv));
    memset(bus, 0, sizeof(*bus));
    free(bus);
}

mcupr_result_t mcupr_i2c_open(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t *dev, int addr)
{
    if (bus == NULL || bus->data == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }

    struct linuxdev_i2c_data *priv = (struct linuxdev_i2c_data *)bus->data;
    char path[32];
    snprintf(path, sizeof(path), "/dev/i2c-%d", priv->busnum);

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        MCUPR_ERR("%s: Can't open i2c device %s", __func__, path);
        return MCUPR_RES_NODEV;
    }
    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        MCUPR_ERR("%s: ioctl I2C_SLAVE failed", __func__);
        close(fd);
        return MCUPR_RES_IO_ERROR;
    }
    *dev = fd;

    return MCUPR_RES_OK;
}

void mcupr_i2c_close(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev)
{
    if (bus == NULL || bus->data == NULL) {
        return;
    }
    if (0 <= dev) {
        close(dev);
    }
}

int mcupr_i2c_write(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev, const uint8_t *data, uint32_t size)
{
    if (bus == NULL || bus->data == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    struct linuxdev_i2c_data *priv = (struct linuxdev_i2c_data *)bus->data;
    if (dev < 0) {
        return MCUPR_RES_IO_ERROR;
    }
    ssize_t ret = write(dev, data, size);
    if (ret < 0) {
        MCUPR_DBG("%s: write failed", __func__);
        return MCUPR_RES_IO_ERROR;
    }
    return (int)ret;
}

int mcupr_i2c_read(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev, uint8_t *data, uint32_t size)
{
    if (bus == NULL || bus->data == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    struct linuxdev_i2c_data *priv = (struct linuxdev_i2c_data *)bus->data;
    if (dev < 0) {
        return MCUPR_RES_IO_ERROR;
    }

    ssize_t ret = read(dev, data, size);
    if (ret < 0) {
        MCUPR_DBG("%s: read failed", __func__);
        return MCUPR_RES_IO_ERROR;
    }
    return (int)ret;
}

#if 0
/*=================================================================================================
 * SPI API (via /dev/spidevX.Y)
 */

struct linuxdev_spi_data {
    int fd;
    int busnum;
};

mcupr_result_t mcupr_spi_bus_create(mcupr_spi_bus_t **bus, mcupr_gpio_chip_params_t *params)
{
}

void mcupr_spi_release(mcupr_spi_bus_t *bus)
{
}

int mcupr_spi_open(mcupr_spi_bus_t *bus, int bus_number, int cs_number, int mode, int speed_hz)
{
    if (!bus) return -1;

    // Close if already open
    if (bus->fd > 0) {
        close(bus->fd);
        bus->fd = -1;
    }

    char path[32];
    snprintf(path, sizeof(path), "/dev/spidev%d.%d", bus_number, cs_number);

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        perror("open spidev");
        return -1;
    }
    uint8_t spi_mode = (uint8_t)mode;
    if (ioctl(fd, SPI_IOC_WR_MODE, &spi_mode) < 0) {
        perror("ioctl SPI_IOC_WR_MODE");
        close(fd);
        return -1;
    }

    uint32_t spi_speed = (uint32_t)speed_hz;
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0) {
        perror("ioctl SPI_IOC_WR_MAX_SPEED_HZ");
        close(fd);
        return -1;
    }

    bus->fd  = fd;
    bus->bus = bus_number;
    return 0;
}

int mcupr_spi_close(mcupr_spi_bus_t *bus)
{
    if (!bus) return -1;
    if (bus->fd >= 0) {
        close(bus->fd);
        bus->fd = -1;
    }
    return 0;
}

int mcupr_spi_transfer(mcupr_spi_bus_t *bus, const uint8_t *tx, uint8_t *rx, size_t length)
{
    if (!bus || bus->fd < 0) return -1;

    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));

    tr.tx_buf = (unsigned long)tx;  // cast if needed for 32-bit
    tr.rx_buf = (unsigned long)rx;
    tr.len    = length;
    // Other fields default to current mode, bits, speed, etc.

    int ret = ioctl(bus->fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 0) {
        perror("spi transfer");
        return -1;
    }
    // ret is typically the total # of bytes transferred
    return ret;
}
#endif /* 0 */

/*=================================================================================================
 * Helper: sysfs GPIO
 */
static int sysfs_gpio_export(int pin) {
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        // Already exported or insufficient permissions
        return -1;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", pin);
    if (write(fd, buf, strlen(buf)) < 0) {
        // Possibly already exported
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

static int sysfs_gpio_unexport(int pin) {
    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        return -1;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", pin);
    if (write(fd, buf, strlen(buf)) < 0) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

static int sysfs_gpio_set_dir(int pin, int is_output) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);

    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        return -1;
    }
    if (is_output) {
        write(fd, "out", 3);
    } else {
        write(fd, "in", 2);
    }
    close(fd);
    return 0;
}

static int sysfs_gpio_write_value(int pin, int value)
{
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);

    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        return -1;
    }
    if (value) {
        write(fd, "1", 1);
    } else {
        write(fd, "0", 1);
    }
    close(fd);
    return 0;
}

static int sysfs_gpio_read_value(int pin) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    char buf[4];
    if (read(fd, buf, sizeof(buf)) < 0) {
        close(fd);
        return -1;
    }
    close(fd);
    return (buf[0] == '1') ? 1 : 0;
}

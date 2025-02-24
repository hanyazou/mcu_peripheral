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
#include <sys/stat.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <linux/spi/spidev.h>

#include "utils.h"
#include <mcu_peripheral/mcu_peripheral.h>
#include <mcu_peripheral/log.h>

/*
 * Implementation for linux device interfaces:
 *  - GPIO via sysfs (/sys/class/gpio)
 *  - I2C via /dev/i2c-N
 *  - SPI via /dev/spidevN.M
 *
 */

static mcupr_result_t sysfs_gpio_export(int pin);
static mcupr_result_t sysfs_gpio_unexport(int pin);
static mcupr_result_t sysfs_gpio_set_dir(int pin, int is_output);
static mcupr_result_t sysfs_gpio_write_value(int pin, int value);
static mcupr_result_t sysfs_gpio_unexport(int pin);
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

mcupr_result_t mcupr_gpio_open(mcupr_gpio_chip_t *chip, mcupr_gpio_device_t *dev, int pin,
                               mcupr_gpio_mode_t mode)
{
    (void)chip; // not used in sysfs example
    mcupr_result_t result = MCUPR_RES_UNKNOWN;

    result = sysfs_gpio_export(pin);
    if (result != MCUPR_RES_OK) {
        MCUPR_ERR("%s: failed to export", __func__);
        return result;
    }

    *dev = pin;

    return sysfs_gpio_set_dir(pin, mode == MCUPR_GPIO_MODE_OUTPUT);
}

void mcupr_gpio_close(mcupr_gpio_chip_t *chip, mcupr_gpio_device_t dev)
{
    (void)chip;
    (void)dev;
}

/* Write value (0 or 1) */
void mcupr_gpio_write(mcupr_gpio_chip_t *chip, mcupr_gpio_device_t dev, int value)
{
    (void)chip; // not used
    sysfs_gpio_write_value((int)dev, value);
}

/* Read the pin value (0 or 1, -1 on error) */
int mcupr_gpio_read(mcupr_gpio_chip_t *chip, mcupr_gpio_device_t dev)
{
    (void)chip; // not used
    return sysfs_gpio_read_value((int)dev);
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

/*=================================================================================================
 * SPI API (via /dev/spidevX.Y)
 */

struct linuxdev_spi_data {
    int dummy;
};

mcupr_result_t mcupr_spi_bus_create(mcupr_spi_bus_t **busp, mcupr_spi_bus_params_t *params)
{
    mcupr_result_t res;
    mcupr_spi_bus_t *bus;

    /* Allocate bus object */
    res = MCUPR_ALLOC_OBJECT(bus, mcupr_spi_bus_t, data, mcupr_spi_bus_t);
    if (res != MCUPR_RES_OK) {
        return res;
    }

    bus->params = *params;
    struct linuxdev_spi_data *priv = (struct linuxdev_spi_data *)bus->data;
    if (bus->params.busnum == MCUPR_UNSPECIFIED) {
        bus->params.busnum = 0;
    }
    *busp = bus;

    return MCUPR_RES_OK;
}

void mcupr_spi_bus_release(mcupr_spi_bus_t *bus)
{
    mcupr_release_object(bus);
}

mcupr_result_t mcupr_spi_open(mcupr_spi_bus_t *bus, mcupr_spi_device_t *dev, int csnum)
{
    if (csnum == MCUPR_UNSPECIFIED) {
        char *env = getenv("MCUPR_SPI_BUSNUM");
        if (env != NULL) {
            MCUPR_INF("%s: cs number is \"%s\"", __func__, env);
            csnum = strtol(env, NULL, 0);
        }
    }

    int fd;
    struct linuxdev_spi_data *priv = (struct linuxdev_spi_data *)bus->data;
    char path[32];

    snprintf(path, sizeof(path), "/dev/spidev%d.%d", bus->params.busnum, csnum);
    fd = open(path, O_RDWR);
    if (fd < 0) {
        MCUPR_ERR("%s: Can' open %s, %s", __func__, path, strerror(errno));
        return MCUPR_RES_IO_ERROR;
    }

    uint8_t spi_mode = (uint8_t)bus->params.mode;
    if (ioctl(fd, SPI_IOC_WR_MODE, &spi_mode) < 0) {
        MCUPR_ERR("%s: ioctl SPI_IOC_WR_MODE, %s", __func__, strerror(errno));
        close(fd);
        return MCUPR_RES_IO_ERROR;
    }

    uint32_t spi_speed = (uint32_t)bus->params.speed;
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0) {
        MCUPR_ERR("%s: ioctl SPI_IOC_WR_MAX_SPEED_HZ, %s", __func__, strerror(errno));
        close(fd);
        return MCUPR_RES_IO_ERROR;
    }

    *dev  = fd;

    return MCUPR_RES_OK;
}

void mcupr_spi_close(mcupr_spi_bus_t *bus, mcupr_spi_device_t dev)
{
    close(dev);
}

int mcupr_spi_transfer(mcupr_spi_bus_t *bus, mcupr_spi_device_t dev,
                       const uint8_t *tx_data, uint8_t *rx_data, int length)
{
    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));

    tr.tx_buf = (unsigned long)tx_data;  // cast if needed for 32-bit
    tr.rx_buf = (unsigned long)rx_data;
    tr.len    = length;
    // Other fields default to current mode, bits, speed, etc.

    int ret = ioctl(dev, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 0) {
        MCUPR_ERR("%s: ioctl SPI_IOC_MESSAGE(1), %s", __func__, strerror(errno));
        return MCUPR_RES_IO_ERROR;
    }

    // ret is typically the total # of bytes transferred
    return ret;
}

/*=================================================================================================
 * Helper: sysfs GPIO
 */
static mcupr_result_t sysfs_gpio_export(int pin) {
    char path[64];

    snprintf(path, sizeof(path), "/sys/class/gpio/export");
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        MCUPR_ERR("%s: Can't open %s, %s", __func__, path, strerror(errno));
        return MCUPR_RES_IO_ERROR;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", pin);
    write(fd, buf, strlen(buf));
    close(fd);

    struct stat st;
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", pin);
    if (stat(path, &st) != 0) {
        MCUPR_ERR("%s: Can't stat %s, %s", __func__, path, strerror(errno));
        return MCUPR_RES_IO_ERROR;
    }

    return MCUPR_RES_OK;
}

static mcupr_result_t sysfs_gpio_unexport(int pin) {
    char *path = "/sys/class/gpio/unexport";
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        MCUPR_ERR("%s: Can't open %s, %s", __func__, path, strerror(errno));
        return MCUPR_RES_IO_ERROR;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", pin);
    if (write(fd, buf, strlen(buf)) < 0) {
        MCUPR_ERR("%s: Can' write %s, %s", __func__, path, strerror(errno));
        close(fd);
        return MCUPR_RES_IO_ERROR;
    }
    close(fd);
    return MCUPR_RES_OK;
}

static mcupr_result_t sysfs_gpio_set_dir(int pin, int is_output) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);

    MCUPR_VBS("%s: open(%s)", __func__, path);
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        MCUPR_ERR("%s: Can't open %s, %s", __func__, path, strerror(errno));
        return MCUPR_RES_IO_ERROR;
    }
    if (is_output) {
        MCUPR_DBG("%s: %s out", __func__, path);
        write(fd, "out", 3);
    } else {
        MCUPR_DBG("%s: %s in", __func__, path);
        write(fd, "in", 2);
    }
    MCUPR_VBS("%s: close(%s)", __func__, path);
    close(fd);
    return MCUPR_RES_OK;
}

static mcupr_result_t sysfs_gpio_write_value(int pin, int value)
{
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);

    MCUPR_VBS("%s: open(%s)", __func__, path);
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        MCUPR_ERR("%s: Can't open %s, %s", __func__, path, strerror(errno));
        return MCUPR_RES_IO_ERROR;
    }
    if (value) {
        MCUPR_DBG("%s: write(%s, 1)", __func__, path);
        write(fd, "1", 1);
    } else {
        MCUPR_DBG("%s: write(%s, 0)", __func__, path);
        write(fd, "0", 1);
    }
    MCUPR_VBS("%s: close(%s)", __func__, path);
    close(fd);
    return MCUPR_RES_OK;
}

static int sysfs_gpio_read_value(int pin) {
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        MCUPR_ERR("%s: Can't open %s, %s", __func__, path, strerror(errno));
        return MCUPR_RES_IO_ERROR;
    }
    char buf[4];
    if (read(fd, buf, sizeof(buf)) < 0) {
        close(fd);
        return MCUPR_RES_IO_ERROR;
    }
    close(fd);
    return (buf[0] == '1') ? 1 : 0;
}

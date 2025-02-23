/*
 * MIT License
 *
 * Copyright (c) 2025 hanyazou
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

#ifndef MCU_PERIPHERAL_H__
#define MCU_PERIPHERAL_H__

/*
 * mcupr_wrapper.h
 *
 * This header file provides a unified C API that wraps GPIO / I2C / SPI functions
 * for various platforms (Arduino, STM32CubeMX HAL, amd64 Linux and Raspberry Pi.
 */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MCUPR_UNSPECIFIED 0x80000000

typedef enum mcupr_result_e {
    MCUPR_RES_OK = 0,
    MCUPR_RES_UNKNOWN = -1,
    MCUPR_RES_INVALID_OBJ = -2,
    MCUPR_RES_INVALID_HANDLE = -3,
    MCUPR_RES_INVALID_NAME = -4,
    MCUPR_RES_INVALID_ARGUMENT = -5,
    MCUPR_RES_INVALID_PARAM = -6,
    MCUPR_RES_BACKEND_FAILURE = -7,
    MCUPR_RES_COMMUNICATION_ERROR = -8,
    MCUPR_RES_BUSY = -9,
    MCUPR_RES_NOMEM = -10,
    MCUPR_RES_NODEV = -11,
    MCUPR_RES_IO_ERROR = -12,
} mcupr_result_t;

void mcupr_initialize(void);
char *mcupr_error(int errno);

/* =================================================================================================
 * GPIO Section
 */

/* GPIO pin modes */
typedef enum {
    MCUPR_GPIO_MODE_INPUT = 0,
    MCUPR_GPIO_MODE_OUTPUT,
    MCUPR_GPIO_MODE_INPUT_PULLUP,
    MCUPR_GPIO_MODE_INPUT_PULLDOWN
} mcupr_gpio_mode_t;

/* GPIO drive strength */
typedef enum {
    MCUPR_GPIO_DRIVE_DEFAULT = 0,
    MCUPR_GPIO_DRIVE_LOW,
    MCUPR_GPIO_DRIVE_MEDIUM,
    MCUPR_GPIO_DRIVE_HIGH
} mcupr_gpio_drive_t;

/* GPIO interrupt edges */
typedef enum {
    MCUPR_GPIO_INT_NONE = 0,
    MCUPR_GPIO_INT_RISING,
    MCUPR_GPIO_INT_FALLING,
    MCUPR_GPIO_INT_BOTH
} mcupr_gpio_int_edge_t;

typedef struct mcupr_gpio_chip_s {
    void *data;
}mcupr_gpio_chip_t;
typedef int mcupr_gpio_device_t;
typedef struct mcupr_gpio_chip_params_s {
    int chip; /* Controller number for platforms with multiple controllers */
} mcupr_gpio_chip_params_t;

void mcupr_gpio_init_params(mcupr_gpio_chip_params_t *params);
mcupr_result_t mcupr_gpio_chip_create(mcupr_gpio_chip_t **chip, mcupr_gpio_chip_params_t *params);
void mcupr_gpio_chip_release(mcupr_gpio_chip_t *chip);

/*
 * Initialize a GPIO pin.
 * pin      : GPIO number (platform-dependent)
 * mode     : mcupr_gpio_mode_t
 */
mcupr_result_t mcupr_gpio_open(mcupr_gpio_chip_t *chip, mcupr_gpio_device_t *dev, int pin,
			       mcupr_gpio_mode_t mode);
void mcupr_gpio_close(mcupr_gpio_chip_t *chip, mcupr_gpio_device_t dev);

/*
 * Read a GPIO pin state.
 * Returns: 0 or 1 (error handling is implementation-dependent)
 */
int mcupr_gpio_read(mcupr_gpio_chip_t *chip, mcupr_gpio_device_t dev);

/*
 * Write to a GPIO pin.
 * value : 0 or 1
 */
void mcupr_gpio_write(mcupr_gpio_chip_t *chip, int pin, mcupr_gpio_device_t dev);

/*
 * Set GPIO drive strength.
 */
void mcupr_gpio_set_drive_strength(mcupr_gpio_chip_t *chip, mcupr_gpio_device_t dev,
				   mcupr_gpio_drive_t drive);

/*
 * Register a GPIO interrupt handler.
 * edge      : Interrupt edge (RISING, FALLING, BOTH)
 * callback  : Function pointer invoked on interrupt
 * user_data : User data passed to the callback
 */
typedef void (*mcupr_gpio_isr_t)(mcupr_gpio_chip_t *chip, int pin, void *user_data);

mcupr_result_t  mcupr_gpio_attach_interrupt(mcupr_gpio_chip_t *chip, int pin,
                                            mcupr_gpio_int_edge_t edge,
                                            mcupr_gpio_isr_t callback,
                                            void *user_data);

/*
 * Disable the interrupt and detach the ISR.
 */
void mcupr_gpio_detach_interrupt(mcupr_gpio_chip_t *chip, int pin);

/* =================================================================================================
 * I2C Section
 */

typedef struct mcupr_i2c_bus_s {
    void *data;
} mcupr_i2c_bus_t;
typedef int mcupr_i2c_device_t;
typedef struct mcupr_i2c_bus_params_s {
    uint32_t busnum; /* Bus number for platforms with multiple i2c buses */
    uint32_t freq;
} mcupr_i2c_bus_params_t;

void mcupr_i2c_init_params(mcupr_i2c_bus_params_t *params);
mcupr_result_t mcupr_i2c_bus_create(mcupr_i2c_bus_t **bus, const mcupr_i2c_bus_params_t *params);
void mcupr_i2c_bus_release(mcupr_i2c_bus_t *bus);

/*
 * Initialize I2C.
 * bus     : I2C bus
 * address : I2C address (7-bit)
 */
mcupr_result_t mcupr_i2c_open(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t *dev, int address);
void mcupr_i2c_close(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev);

/*
 * I2C read (burst read).
 * data   : Buffer for incoming data
 * length : Number of bytes to read
 * Returns: Number of bytes actually read (error handling is implementation-dependent)
 */
int mcupr_i2c_read(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev, uint8_t *data, uint32_t length);

/*
 * I2C write (burst write).
 * data   : Buffer containing data to write
 * length : Number of bytes to write
 * Returns: Number of bytes actually written
 */
int mcupr_i2c_write(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev, const uint8_t *data, uint32_t length);

/*
 * Dynamically set I2C clock frequency (if platform supports it).
 */
mcupr_result_t mcupr_i2c_set_freq(mcupr_i2c_bus_t *bus, uint32_t freq);

/*
 * Enable or disable I2C clock stretching (hardware-dependent).
 * enable : 1 = enable, 0 = disable
 */
mcupr_result_t mcupr_i2c_set_clock_stretch(mcupr_i2c_bus_t *bus, int enable);

/* =================================================================================================
 * SPI Section
 */

/* SPI mode (CPOL, CPHA) */
typedef enum {
    MCUPR_SPI_MODE0 = 0, /* CPOL=0, CPHA=0 */
    MCUPR_SPI_MODE1,     /* CPOL=0, CPHA=1 */
    MCUPR_SPI_MODE2,     /* CPOL=1, CPHA=0 */
    MCUPR_SPI_MODE3      /* CPOL=1, CPHA=1 */
} mcupr_spi_mode_t;

typedef struct mcupr_spi_bus_s mcupr_spi_bus_t;
typedef struct mcupr_spi_bus_params_s {
    int busnum; /* Bus number for platforms with multiple i2c buses */
    uint32_t speed;
    mcupr_spi_mode_t mode;
} mcupr_spi_bus_params_t;

void mcupr_spi_init_params(mcupr_gpio_chip_params_t *params);
mcupr_result_t mcupr_spi_bus_create(mcupr_spi_bus_t **bus, mcupr_gpio_chip_params_t *params);
void mcupr_spi_release(mcupr_spi_bus_t *bus);

/*
 * SPI transfer (full-duplex).
 * tx_data, rx_data: TX/RX buffers (rx_data can be NULL if TX-only)
 * length          : Number of bytes
 * Returns         : Number of bytes actually transferred
 */
int mcupr_spi_transfer(mcupr_spi_bus_t *bus,
                       const uint8_t *tx_data,
                       uint8_t *rx_data,
                       uint32_t length);

/*
 * Dynamically set SPI clock speed (if platform supports it).
 */
mcupr_result_t mcupr_spi_set_speed(mcupr_spi_bus_t *bus, uint32_t speed);

/*
 * Dynamically set SPI mode (if platform supports it).
 */
mcupr_result_t mcupr_spi_set_mode(mcupr_spi_bus_t *bus, mcupr_spi_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif  /* MCU_PERIPHERAL_H__ */

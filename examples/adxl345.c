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
#include <unistd.h>
#include <mcu_peripheral/mcu_peripheral.h>

// Device ID
#define ADXL345_REG_DEVID          0x00  // Device ID (should always be 0xe5)

// Tap and offset control
#define ADXL345_REG_THRESH_TAP     0x1d  // Tap threshold
#define ADXL345_REG_OFSX           0x1e  // X-axis offset
#define ADXL345_REG_OFSY           0x1f  // Y-axis offset
#define ADXL345_REG_OFSZ           0x20  // Z-axis offset
#define ADXL345_REG_DUR            0x21  // Tap duration
#define ADXL345_REG_LATENT         0x22  // Tap latency
#define ADXL345_REG_WINDOW         0x23  // Tap window

// Activity and inactivity control
#define ADXL345_REG_THRESH_ACT     0x24  // Activity threshold
#define ADXL345_REG_THRESH_INACT   0x25  // Inactivity threshold
#define ADXL345_REG_TIME_INACT     0x26  // Inactivity time
#define ADXL345_REG_ACT_INACT_CTL  0x27  // Activity/inactivity control

// Free-fall detection
#define ADXL345_REG_THRESH_FF      0x28  // Free-fall threshold
#define ADXL345_REG_TIME_FF        0x29  // Free-fall time

// Tap settings
#define ADXL345_REG_TAP_AXES       0x2a  // Tap axes control
#define ADXL345_REG_ACT_TAP_STATUS 0x2b  // Activity/tap status

// Power and control registers
#define ADXL345_REG_BW_RATE        0x2c  // Data rate and power mode control
#define ADXL345_REG_POWER_CTL      0x2d  // Power control
#define ADXL345_REG_INT_ENABLE     0x2e  // Interrupt enable
#define ADXL345_REG_INT_MAP        0x2f  // Interrupt mapping
#define ADXL345_REG_INT_SOURCE     0x30  // Interrupt source

// Data format and FIFO control
#define ADXL345_REG_DATA_FORMAT    0x31  // Data format control
#define ADXL345_REG_FIFO_CTL       0x38  // FIFO control
#define ADXL345_REG_FIFO_STATUS    0x39  // FIFO status

// Accelerometer data registers (little-endian format)
#define ADXL345_REG_DATAX0         0x32  // X-axis data (LSB)
#define ADXL345_REG_DATAX1         0x33  // X-axis data (MSB)
#define ADXL345_REG_DATAY0         0x34  // Y-axis data (LSB)
#define ADXL345_REG_DATAY1         0x35  // Y-axis data (MSB)
#define ADXL345_REG_DATAZ0         0x36  // Z-axis data (LSB)
#define ADXL345_REG_DATAZ1         0x37  // Z-axis data (MSB)

uint8_t adxl345_read(mcupr_spi_bus_t *bus, mcupr_spi_device_t dev, int reg);
void adxl345_write(mcupr_spi_bus_t *bus, mcupr_spi_device_t dev, int reg, uint8_t value);
void adxl345_setup_double_tap(mcupr_spi_bus_t *bus, mcupr_spi_device_t dev);
int16_t adxl345_read_axis(mcupr_spi_bus_t *bus, mcupr_spi_device_t dev, uint8_t reg);

int main(int argc, char *argv[])
{
    mcupr_result_t result;
    mcupr_spi_bus_t *bus;
    mcupr_spi_device_t dev;
    mcupr_spi_bus_params_t params;
    int csnum = 0;

    printf("SPI ADXL345 Test Start\n");
    mcupr_initialize();

    mcupr_spi_init_params(&params);
    params.mode = MCUPR_SPI_MODE3;
    result = mcupr_spi_bus_create(&bus, &params);
    if (result != MCUPR_RES_OK) {
        exit(1);
    }
    result = mcupr_spi_open(bus, &dev, csnum);
    if (result != MCUPR_RES_OK) {
        exit(1);
    }

    // Read the Device ID (should be 0xe5 if successful)
    uint8_t devid = adxl345_read(bus, dev, ADXL345_REG_DEVID);
    if (devid != 0xe5) {
        fprintf(stderr, "Error: ADXL345 not detected! Read 0x%02x\n", devid);
        exit(1);
    }
    printf("ADXL345 detected! Device ID: 0x%02x\n", devid);

    // Enable measurement mode
    adxl345_setup_double_tap(bus, dev);

    printf("Reading acceleration data...\n");
    printf("Double tap to exit.\n");

    // Initialize previous values
    int16_t prev_x = 0, prev_y = 0, prev_z = 0;
    const int thresh = 10;  // Threshold to consider as "significant change"

    while (1) {
        // Read acceleration values
        int16_t x = adxl345_read_axis(bus, dev, ADXL345_REG_DATAX0);
        int16_t y = adxl345_read_axis(bus, dev, ADXL345_REG_DATAY0);
        int16_t z = adxl345_read_axis(bus, dev, ADXL345_REG_DATAZ0);

        // Check if any axis has changed significantly
        if (abs(x - prev_x) > thresh || abs(y - prev_y) > thresh || abs(z - prev_z) > thresh) {
            printf("X: %6d, Y: %6d, Z: %6d\n", x, y, z);
            prev_x = x;
            prev_y = y;
            prev_z = z;
        }

        // Read the interrupt source register
        uint8_t int_source = adxl345_read(bus, dev, ADXL345_REG_INT_SOURCE);

        // Check if a double tap was detected (bit 5 = 0x20)
        if (int_source & 0x20) {
            printf("Double tap detected. Exiting.\n");
            break;
        }

        usleep(100000);  // Sleep for 100ms before next reading
    }

    mcupr_spi_close(bus, dev);
    mcupr_spi_bus_release(bus);

    return 0;
}

uint8_t adxl345_read(mcupr_spi_bus_t *bus, mcupr_spi_device_t dev, int reg)
{
    int res;
    uint8_t tx[2] = { reg | 0x80, 0x00 };
    uint8_t rx[2] = { 0 };

    res = mcupr_spi_transfer(bus, dev, tx, rx, 2);
    if (res != 2) {
        printf("SPI transfer failed");
        return 0xFF;
    }

    return rx[1];  // return the received byte
}

void adxl345_write(mcupr_spi_bus_t *bus, mcupr_spi_device_t dev, int reg, uint8_t value)
{
    int res;
    uint8_t tx[2] = { reg & 0x7F, value };

    res = mcupr_spi_transfer(bus, dev, tx, NULL, 2);
    if (res != 2) {
        printf("SPI transfer failed");
    }
}

// Initialize ADXL345 for double-tap detection
void adxl345_setup_double_tap(mcupr_spi_bus_t *bus, mcupr_spi_device_t dev)
{
    // Enable measurement mode
    adxl345_write(bus, dev, ADXL345_REG_POWER_CTL, 0x08);
    usleep(10000);  // Wait 10ms

    // Verify if the setting has been applied
    uint8_t power_ctl = adxl345_read(bus, dev, ADXL345_REG_POWER_CTL);
    printf("Enable measurement mode: 0x%02x\n", power_ctl);

    // Set tap threshold (higher value = stronger tap required)
    adxl345_write(bus, dev, ADXL345_REG_THRESH_TAP, 0x30);  // Example: ~3g

    // Set tap duration (how long acceleration must be maintained to be detected)
    adxl345_write(bus, dev, ADXL345_REG_DUR, 0x10);  // Example: ~10ms

    // Set latency time between taps (time between first and second tap)
    adxl345_write(bus, dev, ADXL345_REG_LATENT, 0x20);  // Example: ~40ms

    // Set window time (max time between first and second tap)
    adxl345_write(bus, dev, ADXL345_REG_WINDOW, 0x96);  // Example: ~150ms

    // Enable double-tap detection on all axes (X, Y, Z)
    adxl345_write(bus, dev, ADXL345_REG_TAP_AXES, 0x07);  // 0x07 = Enable X, Y, Z

    // Enable double-tap interrupt
    adxl345_write(bus, dev, ADXL345_REG_INT_ENABLE, 0x20);  // 0x20 = Enable double-tap interrupt
}

// Reads a 16-bit signed value from two consecutive ADXL345 registers
int16_t adxl345_read_axis(mcupr_spi_bus_t *bus, mcupr_spi_device_t dev, uint8_t reg)
{
    uint8_t low = adxl345_read(bus, dev, reg);
    uint8_t high = adxl345_read(bus, dev, reg + 1);
    return (int16_t)((high << 8) | low);  // Convert to signed 16-bit integer
}

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
#include <math.h>

#include <mcu_peripheral/mcu_peripheral.h>

#define TLS2561_I2C_ADDR 0x39

#define TLS2561_REG_COMMAND_CMD         (1 << 7)
#define TLS2561_REG_COMMAND_CLEAR       (1 << 6)
#define TLS2561_REG_COMMAND_WORD        (1 << 5)
#define TLS2561_REG_COMMAND_BLOCK       (1 << 4)
#define TLS2561_REG_COMMAND_ADDR(a)     (a)

#define TLS2561_REG_CONTROL     0x00
#define TLS2561_REG_CONTROL_POWER_ON    0x3
#define TLS2561_REG_CONTROL_POWER_OFF   0x0

#define TLS2561_REG_TIMING      0x01
#define TLS2561_REG_TIMING_GAIN_1X      (0 << 4)
#define TLS2561_REG_TIMING_GAIN_16X     (1 << 4)
#define TLS2561_REG_TIMING_MANUAL_START (1 << 3)
#define TLS2561_REG_TIMING_MANUAL_STOP  (0 << 3)
#define TLS2561_REG_TIMING_INTEG_13ms   0x0
#define TLS2561_REG_TIMING_INTEG_101ms  0x1
#define TLS2561_REG_TIMING_INTEG_402ms  0x2
#define TLS2561_REG_TIMING_INTEG_MANUAL 0x3

#define TLS2561_REG_DATA0LOW    0x0c
#define TLS2561_REG_DATA0HIGH   0x0d
#define TLS2561_REG_DATA1LOW    0x0e
#define TLS2561_REG_DATA1HIGH   0x0f

unsigned char tsl2561_read(mcupr_i2c_bus_t *i2c_bus, int handle, int reg)
{
    int n;
    unsigned char buf = TLS2561_REG_COMMAND_CMD | reg;
    mcupr_i2c_write(i2c_bus, handle, &buf, 1);

    if (mcupr_i2c_read(i2c_bus, handle, &buf, 1) != 1) {
        return 0xff;
    }

    return buf;
}

void tsl2561_write(mcupr_i2c_bus_t *i2c_bus, int handle, int reg, unsigned char value)
{
    int n;
    unsigned char buf[2];

    buf[0] = TLS2561_REG_COMMAND_CMD | reg;
    buf[1] = value;
    mcupr_i2c_write(i2c_bus, handle, buf, 2);
}

int main(int argc, char *argv[])
{
    mcupr_result_t result;
    mcupr_i2c_bus_t *i2c_bus;
    mcupr_i2c_device_t i2c_dev;
    mcupr_i2c_bus_params_t i2c_bus_params;

    mcupr_initialize();

    mcupr_i2c_init_params(&i2c_bus_params);
    result = mcupr_i2c_bus_create(&i2c_bus, &i2c_bus_params);
    if (result != MCUPR_RES_OK) {
        exit(1);
    }
    result = mcupr_i2c_open(i2c_bus, &i2c_dev, TLS2561_I2C_ADDR);
    if (result != MCUPR_RES_OK) {
        exit(1);
    }

    /* Control Register (0h), power on */
    tsl2561_write(i2c_bus, i2c_dev, TLS2561_REG_CONTROL, TLS2561_REG_CONTROL_POWER_ON);

    /* Timing Register (1h), Nominal intefration time 402ms */
    tsl2561_write(i2c_bus, i2c_dev, TLS2561_REG_TIMING, TLS2561_REG_TIMING_INTEG_402ms);

    /* Read ADC Channel Data Registers */
    unsigned char data0low = tsl2561_read(i2c_bus, i2c_dev, TLS2561_REG_DATA0LOW);
    unsigned char data0high = tsl2561_read(i2c_bus, i2c_dev, TLS2561_REG_DATA0HIGH);
    unsigned char data1low = tsl2561_read(i2c_bus, i2c_dev, TLS2561_REG_DATA1LOW);
    unsigned char data1high = tsl2561_read(i2c_bus, i2c_dev, TLS2561_REG_DATA1HIGH);
    float ch0 = (256 * data0high + data0low);
    float ch1 = (256 * data1high + data1low);
    printf("Ch0=%.2f,  Ch1=%.2f\n", ch0, ch1);

    mcupr_i2c_close(i2c_bus, i2c_dev);
    mcupr_i2c_bus_release(i2c_bus);

    exit(0);
}

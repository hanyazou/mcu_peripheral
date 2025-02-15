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

int main(int argc, char *argv[])
{
    mcupr_result_t result;
    mcupr_i2c_bus_t *i2c_bus;
    mcupr_i2c_bus_params_t i2c_bus_params;

    mcupr_initialize();

    mcupr_i2c_init_params(&i2c_bus_params);
    i2c_bus_params.uri = (1 < argc) ? argv[1] : NULL;
    result = mcupr_i2c_bus_create(&i2c_bus, &i2c_bus_params);
    if (result != MCUPR_RES_OK) {
        exit(1);
    }

    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    for (int address = 0x00; address <= 0x7f; address++) {
        if((address & 0x0f) == 0x00) {
            printf("%02x: ", address);
        }
        if (0x08 <= address && address <= 0x77) {
            result = mcupr_i2c_open(i2c_bus, address);
            if (result != MCUPR_RES_OK) {
                printf("   ");
            } else {
                uint8_t tmp;
                if (mcupr_i2c_write(i2c_bus, address, &tmp, 0) < 0) {
                    printf("-- ");
                } else {
                    printf("%02x ", address);
                }
            }
            mcupr_i2c_close(i2c_bus, address);
        } else {
            printf("   ");
        }
        if((address & 0x0f) == 0x0f) {
            printf("\n");
        }
    }

    mcupr_i2c_bus_release(i2c_bus);

    exit(0);
}

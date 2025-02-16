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

#include <string.h>
#include <stdlib.h>
#include <mcu_peripheral/mcu_peripheral.h>
#include <mcu_peripheral/log.h>

void mcupr_initialize(void)
{

}

void mcupr_i2c_init_params(mcupr_i2c_bus_params_t *params)
{
    memset(params, 0, sizeof(*params));
    params->freq = 400000; /* 400 KHz */

    char *busnum = getenv("MCUPR_I2C_BUSNUM");
    if (busnum != NULL) {
        MCUPR_INF("%s: bus number is \"%s\"", __func__, busnum);
        params->busnum = strtol(busnum, NULL, 0);
    } else {
        params->busnum = MCUPR_UNSPECIFIED;
    }
}

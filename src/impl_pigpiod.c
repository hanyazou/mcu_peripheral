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

#include <stdlib.h>
#include <string.h>
#include <mcu_peripheral/mcu_peripheral.h>
#include <mcu_peripheral/log.h>
#include <pigpiod_if2.h>

struct pigpiod_i2c_data {
    int pi;
    int busnum;
};

mcupr_result_t mcupr_i2c_open(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t *dev, int addr)
{
    if (bus == NULL || bus->data == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    struct pigpiod_i2c_data *priv = (struct pigpiod_i2c_data *)bus->data;
    int handle = i2c_open(priv->pi, priv->busnum, addr, 0);
    *dev = handle;
    return MCUPR_RES_OK;
}

int mcupr_i2c_read(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev, uint8_t *data, uint32_t size)
{
    if (bus == NULL || bus->data == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    struct pigpiod_i2c_data *priv = (struct pigpiod_i2c_data *)bus->data;
    return i2c_read_device(priv->pi, dev, (char *)data, size);
}

int mcupr_i2c_write(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev, const uint8_t *data, uint32_t size)
{
    if (bus == NULL || bus->data == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    struct pigpiod_i2c_data *priv = (struct pigpiod_i2c_data *)bus->data;
    return i2c_write_device(priv->pi, dev, (char *)data, size);
}

void mcupr_i2c_close(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev)
{
    if (bus == NULL || bus->data == NULL) {
        return;
    }
    struct pigpiod_i2c_data *priv = (struct pigpiod_i2c_data *)bus->data;
    i2c_close(priv->pi, dev);
}

void mcupr_i2c_bus_release(mcupr_i2c_bus_t *bus)
{
    if (bus == NULL || bus->data == NULL) {
        return;
    }
    struct pigpiod_i2c_data *priv = (struct pigpiod_i2c_data *)bus->data;
    pigpio_stop(priv->pi);
    memset(priv, 0, sizeof(*priv));
    memset(bus, 0, sizeof(*bus));
    free(bus);
}

mcupr_result_t mcupr_i2c_bus_create(mcupr_i2c_bus_t **busp, const mcupr_i2c_bus_params_t *params)
{
    int i;
    mcupr_i2c_bus_t *bus;
    mcupr_result_t result = MCUPR_RES_UNKNOWN;

    /* Allocate bus object */
    bus = calloc(1, sizeof(mcupr_i2c_bus_t) + sizeof(struct pigpiod_i2c_data));
    if (bus == NULL) {
        MCUPR_ERR("%s: memory allocation failed", __func__);
        return MCUPR_RES_NOMEM;
    }

    struct pigpiod_i2c_data *priv = (struct pigpiod_i2c_data *)&bus[1];
    bus->data = priv;

    if (params->busnum == MCUPR_UNSPECIFIED) {
        priv->busnum = 1;  /* Raspberry Pi's external I2C pins in the pin header */
    } else {
        priv->busnum = params->busnum;
    }

    char *addr = getenv("MCUPR_IMPL_PIGPIOD_ADDR");
    char *port = getenv("MCUPR_IMPL_PIGPIOD_PORT");

    priv->pi = pigpio_start(addr, port);
    if (priv->pi < 0) {
        free(bus);
        return MCUPR_RES_NODEV;
    }

    MCUPR_INF("%s: addr=%s, port=%s, bus=%d", __func__, addr, port, priv->busnum);
    *busp = bus;

    return MCUPR_RES_OK;
}

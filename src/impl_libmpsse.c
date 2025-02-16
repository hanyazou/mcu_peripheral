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
#include <mcu_peripheral/multi_impl.h>
#include <mcu_peripheral/log.h>
#include <mpsse.h>

#define IMPL_NAME "libmpsse"

#define MIN_ADDR 0x00
#define MAX_ADDR 0x7f
#define VALID_ADDR(addr) ((MIN_ADDR <= (addr) && (addr) <= MAX_ADDR))
#define VALID_HANDLE(hdl) (((hdl) & 0x01) == 0 && (MIN_ADDR * 2 <= (hdl) && (hdl) <= MAX_ADDR * 2))

struct libmpsse_data {
    struct mpsse_context *mpsse;
    int rd_addr;
    int wr_addr;
    int clockspeed;
    int msblsb;
};

static int libmpsse_i2c_open(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t *dev, int addr)
{
    if (bus == NULL || bus->data == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    struct libmpsse_data *priv = (struct libmpsse_data *)bus->data;
    if (priv->mpsse == NULL || !priv->mpsse->open) {
        return MCUPR_RES_INVALID_OBJ;
    }
    if (!VALID_ADDR(addr)) {
        return MCUPR_RES_INVALID_ARGUMENT;
    }

    *dev = (addr << 1);  /* use I2C slave address 0x00 to 0xFE as a handler */

    return MCUPR_RES_OK;
}

static int libmpsse_i2c_read(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev, unsigned char *data, uint32_t size)
{
    if (bus == NULL || bus->data == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    struct libmpsse_data *priv = (struct libmpsse_data *)bus->data;
    if (priv->mpsse == NULL || !priv->mpsse->open || !VALID_HANDLE(dev)) {
        return MCUPR_RES_INVALID_HANDLE;
    }

    int res;
    char rd_addr = (dev | 0x01);
    Start(priv->mpsse);
    if (Write(priv->mpsse, &rd_addr, 1) != MPSSE_OK) {
        res = MCUPR_RES_BACKEND_FAILURE;
        goto wayout;
    }
    if (GetAck(priv->mpsse) != ACK) {
        res = MCUPR_RES_COMMUNICATION_ERROR;
        goto wayout;
    }
    char *read_data = Read(priv->mpsse, size);
    if(read_data == NULL) {
        res = MCUPR_RES_COMMUNICATION_ERROR;
    } else {
        memcpy(data, read_data, size);
        free(read_data);
        res = size;
    }
    SendNacks(priv->mpsse);
    Read(priv->mpsse, 1);
    SendAcks(priv->mpsse);

 wayout:
    Stop(priv->mpsse);

    return res;
}

static int libmpsse_i2c_write(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev, const uint8_t *data, uint32_t size)
{
    if (bus == NULL || bus->data == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    struct libmpsse_data *priv = (struct libmpsse_data *)bus->data;
    if (priv->mpsse == NULL || !priv->mpsse->open || !VALID_HANDLE(dev)) {
        return MCUPR_RES_INVALID_HANDLE;
    }

    int res;
    char wr_addr = (dev | 0x00);
    Start(priv->mpsse);
    if (Write(priv->mpsse, &wr_addr, 1) != MPSSE_OK) {
        res = MCUPR_RES_BACKEND_FAILURE;
        goto wayout;
    }
    if (GetAck(priv->mpsse) != ACK) {
        res = MCUPR_RES_COMMUNICATION_ERROR;
        goto wayout;
    }

    if (0 < size) {
        Write(priv->mpsse, (char*)data, size);
        if (GetAck(priv->mpsse) != ACK) {
            res = MCUPR_RES_COMMUNICATION_ERROR;
            goto wayout;
        }
    }
    res = size;

 wayout:
    Stop(priv->mpsse);

    return res;
}

static void libmpsse_i2c_close(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev)
{
    /* nothing to do here */
}

static void libmpsse_i2c_release(mcupr_i2c_bus_t *bus)
{
    if (bus == NULL || bus->data == NULL) {
        return;
    }
    struct libmpsse_data *priv = (struct libmpsse_data *)bus->data;
    Close(priv->mpsse);
    memset(priv, 0, sizeof(*priv));
    memset(bus, 0, sizeof(*bus));
    free(bus);
}

static mcupr_i2c_bus_t libmpsse_i2c_bus_tmpl = {
    .open = libmpsse_i2c_open,
    .read = libmpsse_i2c_read,
    .write = libmpsse_i2c_write,
    .close = libmpsse_i2c_close,
    .release = libmpsse_i2c_release,
};

static mcupr_result_t libmpsse_i2c_create(mcupr_i2c_bus_t **busp, mcupr_i2c_bus_params_t *params)
{
    int i;
    mcupr_i2c_bus_t *bus;
    mcupr_result_t result = MCUPR_RES_UNKNOWN;

    /* Allocate bus object */
    bus = calloc(1, sizeof(libmpsse_i2c_bus_tmpl) + sizeof(struct libmpsse_data));
    if (bus == NULL) {
        MCUPR_ERR("%s: memory allocation failed", __func__);
        return MCUPR_RES_NOMEM;
    }
    memcpy(bus, &libmpsse_i2c_bus_tmpl, sizeof(libmpsse_i2c_bus_tmpl));

    struct libmpsse_data *priv = (struct libmpsse_data *)&bus[1];
    bus->data = priv;

    priv->clockspeed = params->freq;
    priv->msblsb = MSB;

    priv->mpsse = MPSSE(I2C, priv->clockspeed, priv->msblsb);
    if (priv->mpsse == NULL || !priv->mpsse->open) {
        if (priv->mpsse) {
            Close(priv->mpsse);
        }
        free(bus);
        return MCUPR_RES_BACKEND_FAILURE;
    }

    MCUPR_INF("%s: clockspeed=%d", __func__, priv->clockspeed);
    *busp = bus;

    return MCUPR_RES_OK;
}

static mcupr_i2c_impl_entry_t libmpsse_i2c_entry = {
    .name = IMPL_NAME,
    .create = libmpsse_i2c_create,
};

void mcupr_libmpsse_initialize(void)
{
    mcupr_i2c_bus_register(&libmpsse_i2c_entry);
}

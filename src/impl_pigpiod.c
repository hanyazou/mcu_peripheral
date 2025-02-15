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
#include <mcu_peripheral/uri.h>
#include <mcu_peripheral/log.h>
#include <pigpiod_if2.h>

#define IMPL_NAME "pigpiod"

struct pigpiod_i2c_data {
    int pi;
    int busnum;
};

static mcupr_result_t pigpiod_i2c_open(mcupr_i2c_bus_t *bus, int addr)
{
    if (bus == NULL || bus->data == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    struct pigpiod_i2c_data *priv = (struct pigpiod_i2c_data *)bus->data;
    return i2c_open(priv->pi, priv->busnum, addr, 0);
}

static int pigpiod_i2c_read(mcupr_i2c_bus_t *bus, int address, uint8_t *data, uint32_t size)
{
    if (bus == NULL || bus->data == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    struct pigpiod_i2c_data *priv = (struct pigpiod_i2c_data *)bus->data;
    return i2c_read_device(priv->pi, address, (char *)data, size);
}

static int pigpiod_i2c_write(mcupr_i2c_bus_t *bus, int address, const uint8_t *data, uint32_t size)
{
    if (bus == NULL || bus->data == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    struct pigpiod_i2c_data *priv = (struct pigpiod_i2c_data *)bus->data;
    return i2c_write_device(priv->pi, address, (char *)data, size);
}

static void pigpiod_i2c_close(mcupr_i2c_bus_t *bus, int address)
{
    if (bus == NULL || bus->data == NULL) {
        return;
    }
    struct pigpiod_i2c_data *priv = (struct pigpiod_i2c_data *)bus->data;
    i2c_close(priv->pi, address);
}

static void pigpiod_i2c_release(mcupr_i2c_bus_t *bus)
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

static mcupr_i2c_bus_t pigpiod_i2c_bus_tmpl = {
    .open = pigpiod_i2c_open,
    .read = pigpiod_i2c_read,
    .write = pigpiod_i2c_write,
    .close = pigpiod_i2c_close,
    .release = pigpiod_i2c_release,
};

static mcupr_result_t pigpiod_i2c_create(mcupr_i2c_bus_t **busp, mcupr_i2c_bus_params_t *params)
{
    int i;
    char *uri = params->uri;
    mcupr_i2c_bus_t *bus;
    mcupr_result_t result = MCUPR_RES_UNKNOWN;

    /* Allocate bus object */
    bus = calloc(1, sizeof(pigpiod_i2c_bus_tmpl) + sizeof(struct pigpiod_i2c_data));
    if (bus == NULL) {
        MCUPR_ERR("%s: memory allocation failed", __func__);
        return MCUPR_RES_NOMEM;
    }
    memcpy(bus, &pigpiod_i2c_bus_tmpl, sizeof(pigpiod_i2c_bus_tmpl));
    struct pigpiod_i2c_data *priv = (struct pigpiod_i2c_data *)&bus[1];
    bus->data = priv;

    /* Try to connect pigpiod with the library default parameters if URI is not specified */
    if (uri == NULL) {
        priv->pi = pigpio_start(NULL, NULL);
        if (priv->pi < 0) {
            result = MCUPR_RES_INVALID_URI;
            goto not_match;
        }
        priv->busnum = 1;
        *busp = bus;
        return MCUPR_RES_OK;
    }

    char addr[128] = {0};
    char port[8] = {0};
    char *tail;
    char *ptr = uri;

    /* Check the schema in URI if URI is not null */
    if (mcupr_uri_string(&ptr, NULL, 0, IMPL_NAME ":", MCUPR_URI_MATCH_EXACT) <= 0) {
        result = MCUPR_RES_INVALID_URI;
        goto not_match;
    }

    if (0 < mcupr_uri_string(&ptr, NULL, 0, "//", MCUPR_URI_MATCH_EXACT)) {
        mcupr_uri_string(&ptr, addr, sizeof(addr), ":/", MCUPR_URI_UNMATCH_CHARS);
        if (0 < mcupr_uri_string(&ptr, NULL, 0, ":", MCUPR_URI_MATCH_EXACT)) {
            mcupr_uri_string(&ptr, port, sizeof(port), "/", MCUPR_URI_UNMATCH_CHARS);
        }
    }

    if (addr[0] || port[0]) {
        MCUPR_INF("%s: pigpio_start(%s, %s)", __func__, addr, port);

        (void)strtol(port, &tail, 10);
        if (*tail != '\0') {
            result = MCUPR_RES_INVALID_URI;
            goto malformed_uri;
        }
    }
    if (*uri != '\0') {
        mcupr_uri_string(&ptr, NULL, 0, "/", MCUPR_URI_MATCH_EXACT);
        MCUPR_INF("%s: bus number is \"%s\"", __func__, ptr);
        priv->busnum = strtol(ptr, &tail, 10);
        if (*tail != '\0') {
            result = MCUPR_RES_INVALID_URI;
            goto malformed_uri;
        }
    } else {
        priv->busnum = 1;  /* Raspberry Pi's external I2C pins in the pin header */
    }

    MCUPR_INF("%s: addr=%s, port=%s, bus=%d", __func__, addr, port, priv->busnum);
    priv->pi = pigpio_start(*addr ? addr : NULL, *port ? port : NULL);
    if (priv->pi < 0) {
        MCUPR_ERR("%s: pigpio_start(\"%s\", \"%s\") failed", __func__, addr, port);
        result = MCUPR_RES_BACKEND_FAILURE;
        goto error;
    }

    *busp = bus;
    return MCUPR_RES_OK;

 malformed_uri:
    MCUPR_ERR("%s: malformed URI, \"%s\"", __func__, uri);

 error:
 not_match:
    memset(priv, 0, sizeof(*priv));
    memset(bus, 0, sizeof(*bus));
    free(bus);

    return result;
}

static mcupr_i2c_impl_entry_t pigpiod_entry = {
    .name = IMPL_NAME,
    .create = pigpiod_i2c_create,
};

void mcupr_pigpiod_initialize(void)
{
    mcupr_i2c_bus_register(&pigpiod_entry);
}

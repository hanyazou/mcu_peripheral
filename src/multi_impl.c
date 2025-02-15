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
#include <string.h>
#include <mcu_peripheral/multi_impl.h>
#include <mcu_peripheral/log.h>

static mcupr_i2c_impl_entry_t *impl_list = NULL;

#define module(name) extern void mcupr_##name##_initialize(void); \
    void __attribute__((weak)) mcupr_##name##_initialize(void) { }

module(pigpiod)
module(libmpsse)

void mcupr_initialize(void)
{
    mcupr_pigpiod_initialize();
    mcupr_libmpsse_initialize();
}

void mcupr_i2c_bus_register(mcupr_i2c_impl_entry_t *entry)
{
    mcupr_i2c_impl_entry_t **p = &impl_list;

    while (*p) {
        if (*p == entry || ((*p)->name && entry->name && strcmp((*p)->name, entry->name) == 0)) {
            MCUPR_ERR("%s: %s is alredy registered", __func__, entry->name);
            return;
        }
        p = &((*p)->next);
    }
    MCUPR_DBG("%s: %s is registered", __func__, entry->name);
    entry->next = impl_list;
    impl_list = entry;
}

mcupr_result_t mcupr_i2c_bus_create(mcupr_i2c_bus_t **bus, mcupr_i2c_bus_params_t *params)
{
    mcupr_i2c_impl_entry_t *impl = impl_list;
    mcupr_result_t result = MCUPR_RES_BACKEND_FAILURE;

    while (impl) {
        if (impl->create && (result = (*impl->create)(bus, params)) == MCUPR_RES_OK) {
            return MCUPR_RES_OK;
        }
        impl = impl->next;
    }

    MCUPR_WRN("%s: can't create instance for %s", __func__, params->uri);

    return result;
}

mcupr_result_t mcupr_i2c_open(mcupr_i2c_bus_t *bus, int addr)
{
    if (bus == NULL || bus->open == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    return (*bus->open)(bus, addr);
}

int mcupr_i2c_read(mcupr_i2c_bus_t *bus, int address, uint8_t *data, uint32_t length)
{
    if (bus == NULL || bus->read == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    return (*bus->read)(bus, address, data, length);
}

int mcupr_i2c_write(mcupr_i2c_bus_t *bus, int address, const uint8_t *data, uint32_t length)
{
    if (bus == NULL || bus->write == NULL) {
        return MCUPR_RES_INVALID_OBJ;
    }
    return (*bus->write)(bus, address, data, length);
}

void mcupr_i2c_close(mcupr_i2c_bus_t *bus, int handle)
{
    if (bus == NULL || bus->close == NULL) {
        return;
    }
    return (*bus->close)(bus, handle);
}

void mcupr_i2c_bus_release(mcupr_i2c_bus_t *bus)
{
    if (bus == NULL || bus->close == NULL) {
        return;
    }
    return (*bus->release)(bus);
}

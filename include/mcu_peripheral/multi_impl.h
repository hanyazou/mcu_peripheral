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

#ifndef MCU_PERIPHERAL_MULTI_IMPL_H__
#define MCU_PERIPHERAL_MULTI_IMPL_H__

#include <mcu_peripheral/mcu_peripheral.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mcupr_i2c_impl_entry_s {
    char *name;
    struct mcupr_i2c_impl_entry_s *next;
    mcupr_result_t (*create)(mcupr_i2c_bus_t **bus, mcupr_i2c_bus_params_t *params);
} mcupr_i2c_impl_entry_t;

struct mcupr_i2c_bus_s {
    void *data;
    mcupr_result_t (*open)(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t *dev, int address);
    int (*read)(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev, uint8_t *data, uint32_t length);
    int (*write)(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev, const uint8_t *data, uint32_t length);
    void (*close)(mcupr_i2c_bus_t *bus, mcupr_i2c_device_t dev);
    void (*release)(mcupr_i2c_bus_t *bus);
    mcupr_result_t (*set_freq)(mcupr_i2c_bus_t *bus, uint32_t freq);
    mcupr_result_t (*set_clock_stretch)(mcupr_i2c_bus_t *bus, int enable);
};

void mcupr_i2c_bus_register(mcupr_i2c_impl_entry_t *entry);

#endif  /* GPIOW_MULTI_IMPL_H__ */

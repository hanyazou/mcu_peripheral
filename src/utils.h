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

#ifndef MCU_PERIPHERAL_UTILS_H__
#define MCU_PERIPHERAL_UTILS_H__

#include <mcu_peripheral/mcu_peripheral.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MCUPR_ALIGN(x, n) (((x) + ((n) - 1)) & ~((n) - 1))
#define MCUPR_ALLOC_OBJECT(obj, obj_type, data, data_type) \
	mcupr_alloc_object((void**)&(obj), sizeof(obj_type), \
			   offsetof(data_type, data), sizeof(data_type))

mcupr_result_t mcupr_alloc_object(void **obj0, int size0, int offset, int size1);
void mcupr_release_object(void *obj);

#ifdef __cplusplus
}
#endif

#endif  /* MCU_PERIPHERAL_UTILS_H__ */

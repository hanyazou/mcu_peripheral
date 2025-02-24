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
#include "utils.h"
#include <mcu_peripheral/mcu_peripheral.h>
#include <mcu_peripheral/log.h>

typedef struct {
    size_t size;
} mcupr_object_t;

mcupr_result_t mcupr_alloc_object(void **obj0, int size0, int offset, int size1)
{
    uint8_t *buf = NULL;
    int size = MCUPR_ALIGN(sizeof(mcupr_object_t), sizeof(void*));
    size0 = MCUPR_ALIGN(size0, sizeof(void*));
    size1 = MCUPR_ALIGN(size1, sizeof(void*));

    /* Allocate bus object */
    buf = calloc(1, size + size0 + size1);
    if (buf == NULL) {
        MCUPR_ERR("%s: memory allocation failed", __func__);
        return MCUPR_RES_NOMEM;
    }

    mcupr_object_t *obj = (mcupr_object_t *)buf;
    obj->size = size + size0 + size1;
    *obj0 = &buf[size];
    if (0 <size1) {
        *((void**)&buf[size + offset]) = &buf[size + size0];
    }

    return MCUPR_RES_OK;
}

void mcupr_release_object(void *objp)
{
    if (objp == NULL) {
        return;
    }
    int size = MCUPR_ALIGN(sizeof(mcupr_object_t), sizeof(void*));
    mcupr_object_t *obj = (mcupr_object_t *)((uint8_t*)objp - size);

    memset(objp, 0, obj->size);
    free(obj);
}

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

#include <stdarg.h>
#include <stdio.h>
#include <mcu_peripheral/mcu_peripheral.h>
#include <mcu_peripheral/log.h>

mcupr_log_level_t mcupr_log_level = MCUPR_LOG_INFO;

void mcupr_printf(char *fmt, ...)
{
    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    (*mcupr_vprintf_hook)(fmt, arg_ptr);
    va_end(arg_ptr);
}

void mcupr_log_impl(mcupr_log_level_t level, char *fmt, ...)
{
    va_list arg_ptr;
    char *header;
    static char *level_strings[] = {
        [MCUPR_LOG_ERROR] = "Error",
        [MCUPR_LOG_WARN] = "Warning",
        [MCUPR_LOG_INFO] = "Info",
        [MCUPR_LOG_DEBUG] = "D",
        [MCUPR_LOG_VERBOSE] = "V",
    };
    if (mcupr_log_level < level) {
        return;
    }

    if (0 <= level && level < sizeof(level_strings)/sizeof(*level_strings)) {
        header = level_strings[level];
    } else {
        header = "???";
    }

    mcupr_printf("%s: ", header);
    va_start(arg_ptr, fmt);
    (*mcupr_vprintf_hook)(fmt, arg_ptr);
    va_end(arg_ptr);
    mcupr_printf("\n");
}

void (*mcupr_log_hook)(mcupr_log_level_t level, char *fmt, ...) = mcupr_log_impl;
int (*mcupr_vprintf_hook)(const char *format, va_list arg) = vprintf;

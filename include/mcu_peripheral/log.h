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

#ifndef MCU_PERIPHERAL_LOG_H__
#define MCU_PERIPHERAL_LOG_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum mcupr_log_level_e {
    MCUPR_LOG_ERROR,
    MCUPR_LOG_WARN,
    MCUPR_LOG_INFO,
    MCUPR_LOG_DEBUG,
    MCUPR_LOG_VERBOSE,
} mcupr_log_level_t;

#define mcupr_log(level, fmt ...) do { (*mcupr_log_hook)(level, fmt); } while (0)
extern void (*mcupr_log_hook)(mcupr_log_level_t level, char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));
extern void mcupr_printf(char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
extern int (*mcupr_vprintf_hook)(const char *format, va_list arg);
extern mcupr_log_level_t mcupr_log_level;

#ifdef MCUPR_DISABLE_PRINT_ERROR
#define MCUPR_ERR(fmt, args...) do { } while (0)
#else
#define MCUPR_ERR(fmt, args...) \
    mcupr_log(MCUPR_LOG_ERROR, fmt, ##args)
#endif

#ifdef MCUPR_DISABLE_PRINT_WARN
#define MCUPR_WRN(fmt, args...) do { } while (0)
#else
#define MCUPR_WRN(fmt, args...) \
    mcupr_log(MCUPR_LOG_WARN, fmt, ##args)
#endif

#ifdef MCUPR_DISABLE_PRINT_INFO
#define MCUPR_INF(fmt, args...) do { } while (0)
#else
#define MCUPR_INF(fmt, args...) \
    mcupr_log(MCUPR_LOG_INFO, fmt, ##args)
#endif

#ifdef MCUPR_DISABLE_PRINT_DEBUG
#define MCUPR_DBG(fmt, args...) do { } while (0)
#else
#define MCUPR_DBG(fmt, args...) \
    mcupr_log(MCUPR_LOG_DEBUG, fmt, ##args)
#endif

#ifdef MCUPR_DISABLE_PRINT_VERBOSE
#define MCUPR_VBS(fmt, args...) do { } while (0)
#else
#define MCUPR_VBS(fmt, args...) \
    mcupr_log(MCUPR_LOG_VERBOSE, fmt, ##args)
#endif

#ifdef __cplusplus
}
#endif

#endif  /* MCU_PERIPHERAL_LOG_H__ */

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
#include <stdlib.h>
#include <mcu_peripheral/mcu_peripheral.h>
#include <mcu_peripheral/uri.h>
#include <mcu_peripheral/log.h>

static int mcupr_uri_match(char *uri, char **pattern, int flags)
{
    if (*uri == '\0') {
        return 0;
    }
    if ((flags & 0x0f) == MCUPR_URI_MATCH_CHARS) {
        MCUPR_VBS("%s: *uri=%c, pattern=%s, match=%s", __func__, *uri, *pattern,
                  strchr(*pattern, *uri) != NULL ? "true" : "false");
        return strchr(*pattern, *uri) != NULL;
    }
    if ((flags & 0x0f) == MCUPR_URI_UNMATCH_CHARS) {
        MCUPR_VBS("%s: *uri=%c, pattern=%s, unmatch=%s", __func__, *uri,
                  *pattern, strchr(*pattern, *uri) == NULL ? "true" : "false");
        return strchr(*pattern, *uri) == NULL;
    }
    if ((flags & 0x0f) == MCUPR_URI_MATCH_EXACT) {
        if (**pattern == '\0') {
            MCUPR_VBS("%s: *uri=%c, pattern=%s, END of exact match",
                      __func__, *uri, *pattern);
            return 0;
        }
        if (**pattern == *uri) {
            MCUPR_VBS("%s: *uri=%c, pattern=%s, match=true",
                      __func__, *uri, *pattern);
            (*pattern)++;
            return 1;
        } else {
            MCUPR_VBS("%s: *uri=%c, pattern=%s, match=false",
                      __func__, *uri, *pattern);
            return 0;
        }
    }

    return 1;
}

int mcupr_uri_string(char **uri, char *buf, int len, char *pattern, int flags)
{
    int res;

    if (uri == NULL || *uri == NULL) {
        return MCUPR_RES_INVALID_URI;
    }

    char *p = *uri;
    while (mcupr_uri_match(p, &pattern, flags)) {
        if (1 < len && buf != NULL) {
            *buf++ = *p;
            len--;
        }
        p++;
    }
    if (0 < len && buf != NULL) {
        *buf = '\0';
    }
    if ((flags & 0x0f) == MCUPR_URI_MATCH_EXACT && *pattern != '\0') {
        /* not match exactly */
        p = *uri;
    }
    res = p - *uri;
    if ((flags & MCUPR_URI_PRESERVE) == 0) {
        *uri = p;
    }

    return res;
}

int mcupr_uri_integer(char **uri, int *number, int base, int flags)
{
    int res;
    long tmp_num;
    char *tmp_ptr;

    if (uri == NULL || *uri == NULL) {
        return MCUPR_RES_INVALID_URI;
    }

    tmp_num = strtol(*uri, &tmp_ptr, base);
    if (number != NULL) {
        *number = tmp_num;
    }

    res = tmp_ptr - *uri;
    if ((flags & MCUPR_URI_PRESERVE) == 0) {
        *uri = tmp_ptr;
    }

    return res;
}

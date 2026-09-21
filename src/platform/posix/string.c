/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "platform/string.h"

#if defined(__linux__) || defined(VEDAM_OS_LINUX)

#include <string.h>

void vplatform_strlcpy(char *dst, const char *src, size_t dst_size)
{
    size_t i;

    if (dst == NULL || dst_size == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    for (i = 0; i + 1 < dst_size && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

#else
#error "vplatform string: unsupported OS"
#endif

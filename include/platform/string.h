/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef V_PLATFORM_STRING_H
#define V_PLATFORM_STRING_H

#include <stddef.h>

/**
 * @brief Copy at most @p dst_size - 1 bytes and always NUL-terminate when dst_size > 0.
 *
 * @param[out] dst      Destination buffer
 * @param[in]  src      Source string
 * @param[in]  dst_size Size of @p dst, including room for the NUL
 */
void vplatform_strlcpy(char *dst, const char *src, size_t dst_size);

#endif

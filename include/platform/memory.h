/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef V_PLATFORM_MEMORY_H
#define V_PLATFORM_MEMORY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate and zero @p count * @p size bytes.
 *
 * @param[in] count Number of objects
 * @param[in] size  Size of one object
 * @return Pointer on success, NULL on failure.
 */
void *vplatform_calloc(size_t count, size_t size);

/**
 * @brief Release memory from vplatform_calloc or vplatform_malloc.
 *
 * @param[in] ptr Block to release, or NULL
 */
void vplatform_free(void *ptr);

/**
 * @brief Fill @p len bytes at @p dst with @p value.
 *
 * @param[out] dst   Destination
 * @param[in]  value Byte written into each position
 * @param[in]  len   Number of bytes
 */
void vplatform_memset(void *dst, int value, size_t len);

/**
 * @brief Copy @p len bytes from @p src to @p dst.
 *
 * @param[out] dst Destination
 * @param[in]  src Source
 * @param[in]  len Number of bytes
 */
void vplatform_memcpy(void *dst, const void *src, size_t len);

#ifdef __cplusplus
}
#endif

#endif

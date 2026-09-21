/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "platform/memory.h"

#if defined(__linux__) || defined(VEDAM_OS_LINUX)

#include <stdlib.h>
#include <string.h>

void *vplatform_calloc(size_t count, size_t size)
{
    return calloc(count, size);
}

/**
 * @brief Free a block from vplatform_calloc.
 * @param[in] ptr Argument ptr
 */
void vplatform_free(void *ptr)
{
    free(ptr);
}

/**
 * @brief Fill @p n bytes at @p dst with @p c.
 * @param[in] dst Argument dst
 * @param[in] value Argument value
 * @param[in] len Argument len
 */
void vplatform_memset(void *dst, int value, size_t len)
{
    memset(dst, value, len);
}

/**
 * @brief Copy @p n bytes from @p src to @p dst.
 * @param[in] dst Argument dst
 * @param[in] src Argument src
 * @param[in] len Argument len
 */
void vplatform_memcpy(void *dst, const void *src, size_t len)
{
    memcpy(dst, src, len);
}

#else
#error "vplatform memory: unsupported OS"
#endif

/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef V_UTIL_ATTR_H
#define V_UTIL_ATTR_H

#include "vi/types.h"

/**
 * @brief Keep the last entry for each attribute id.
 *
 * @param[in,out] list  Attribute list rewritten in place
 * @param[in]     count Number of entries in @p list
 * @return The number of entries kept.
 */
vuint32_t vattr_collapse_last(v_attr_t *list, vuint32_t count);

/**
 * @brief Heap-copy an attribute list.
 *
 * @param[in] list  Source list
 * @param[in] count Number of entries
 * @return A new list, or NULL when @p count is zero or allocation fails.
 */
v_attr_t *vattr_dup(const v_attr_t *list, vuint32_t count);

/**
 * @brief Free a list from vattr_dup.
 *
 * @param[in] list List from vattr_dup, or NULL
 */
void vattr_free(v_attr_t *list);

#endif

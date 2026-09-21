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
 * @brief Keep the last entry for each attribute id; return the new count.
 */
vuint32_t vattr_collapse_last(v_attr_t *list, vuint32_t count);

/**
 * @brief Heap-copy an attribute list. Returns NULL when count is zero or allocation fails.
 */
v_attr_t *vattr_dup(const v_attr_t *list, vuint32_t count);

/**
 * @brief Free a list from vattr_dup.
 */
void vattr_free(v_attr_t *list);

#endif

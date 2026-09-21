/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "util/attr.h"

#include "platform/memory.h"

/**
 * @brief Keep the last entry for each attribute id; return the new count.
 */
vuint32_t vattr_collapse_last(v_attr_t *list, vuint32_t count)
{
    vuint32_t i;
    vuint32_t j;
    vuint32_t write = 0;

    if (list == NULL) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        int keep = 1;
        for (j = i + 1; j < count; j++) {
            if (list[j].id == list[i].id) {
                keep = 0;
                break;
            }
        }
        if (keep) {
            list[write++] = list[i];
        }
    }
    return write;
}

/**
 * @brief Heap-copy an attribute list.
 */
v_attr_t *vattr_dup(const v_attr_t *list, vuint32_t count)
{
    v_attr_t *copy;

    if (count == 0) {
        return NULL;
    }
    copy = (v_attr_t *)vplatform_calloc((size_t)count, sizeof(v_attr_t));
    if (copy == NULL) {
        return NULL;
    }
    vplatform_memcpy(copy, list, sizeof(v_attr_t) * (size_t)count);
    return copy;
}

/**
 * @brief Free a list from vattr_dup.
 */
void vattr_free(v_attr_t *list)
{
    vplatform_free(list);
}

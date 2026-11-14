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

#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * @brief Write text into the buffer the caller gave in @p attr.
 *
 * On entry value.text.len is the room in value.text.buf. Copies only when
 * @p len fits that room. On return value.text.len is @p len in every case, so
 * a caller that overflowed learns the needed length. When this thread has a
 * write watch and @p attr lies in it, the attribute is marked on every return
 * after it is accepted, including a zero-length write and V_STATUS_BUFFER_OVERFLOW.
 *
 * @param[in,out] attr Attribute whose text buffer receives the bytes
 * @param[in]     src  Bytes to write
 * @param[in]     len  Number of bytes in @p src
 * @return V_STATUS_SUCCESS, V_STATUS_BUFFER_OVERFLOW when @p len exceeds the room,
 *         or V_STATUS_INVALID_PARAMETER when @p attr, or @p src with a length, is NULL.
 */
v_status_t vattr_put_text(v_attr_t *attr, const char *src, vuint32_t len);

/**
 * @brief Write list items into the array the caller gave in @p attr.
 *
 * On entry value.list.count is the room in value.list.items, counted in items.
 * Copies only when @p count fits that room. On return value.list.count is
 * @p count in every case, so a caller that overflowed learns the needed number.
 * When this thread has a write watch and @p attr lies in it, the attribute is
 * marked on every return after it is accepted, including a zero-length write
 * and V_STATUS_BUFFER_OVERFLOW.
 *
 * @param[in,out] attr      Attribute whose item array receives the items
 * @param[in]     items     Items to write, one after another
 * @param[in]     count     Number of items in @p items
 * @param[in]     elem_size Bytes in one item
 * @return V_STATUS_SUCCESS, V_STATUS_BUFFER_OVERFLOW when @p count exceeds the room,
 *         or V_STATUS_INVALID_PARAMETER when @p attr, or @p items with a count, is NULL.
 */
v_status_t vattr_put_list(v_attr_t *attr, const void *items, vuint32_t count, vuint32_t elem_size);

/**
 * @brief Remember writer calls on this thread for @p count attributes at @p base.
 *
 * A later vattr_put_text or vattr_put_list marks an attribute that lies in
 * [base, base + count). A NULL @p base marks nothing. A nested call saves this
 * watch, and vattr_write_unwatch restores it. Four nested watches fit.
 *
 * @param[in] base  First attribute of the range, or NULL
 * @param[in] count Number of attributes in the range
 */
void vattr_write_watch(v_attr_t *base, vuint32_t count);

/**
 * @brief Restore the write watch saved before the matching vattr_write_watch.
 */
void vattr_write_unwatch(void);

/**
 * @brief Return non-zero when the current watch saw a writer accept @p attr.
 *
 * @param[in] attr Attribute to test, or NULL
 * @return Non-zero when a writer accepted @p attr under the current watch.
 *         Zero when no watch is active or @p attr is outside it.
 */
int vattr_was_written(const v_attr_t *attr);

#ifdef __cplusplus
}
#endif

#endif

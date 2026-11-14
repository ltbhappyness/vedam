/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "util/attr.h"

#include "platform/memory.h"

#include <stdint.h>

/**
 * @brief Keep the last entry for each attribute id.
 *
 * @param[in,out] list  Attribute list rewritten in place
 * @param[in]     count Number of entries in @p list
 * @return The number of entries kept.
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
 *
 * @param[in] list  Source list
 * @param[in] count Number of entries
 * @return A new list, or NULL when @p count is zero or allocation fails.
 */
v_attr_t *vattr_dup(const v_attr_t *list, vuint32_t count)
{
    v_attr_t *copy;

    if (count == 0) {
        return NULL;
    }
    copy = vplatform_calloc((size_t)count, sizeof(v_attr_t));
    if (copy == NULL) {
        return NULL;
    }
    vplatform_memcpy(copy, list, sizeof(v_attr_t) * (size_t)count);
    return copy;
}

/**
 * @brief Free a list from vattr_dup.
 *
 * @param[in] list List from vattr_dup, or NULL
 */
void vattr_free(v_attr_t *list)
{
    vplatform_free(list);
}

/* One bit per slot. The server attribute array is 32, and util does not include vti. */
enum { VATTR_WATCH_DEPTH = 4, VATTR_WATCH_SLOTS = 32 };

struct vattr_watch {
    v_attr_t *base;
    vuint32_t count;
    vuint32_t bits;
};

static _Thread_local struct vattr_watch watch_stack[VATTR_WATCH_DEPTH];
static _Thread_local unsigned watch_depth;
static _Thread_local unsigned watch_spare;

/**
 * @brief Report whether @p attr is one of the watched slots in @p frame.
 *
 * @param[in]  frame Current watch
 * @param[in]  attr  Attribute to place
 * @param[out] index Slot index when the return is non-zero
 * @return Non-zero when @p attr is one of the first VATTR_WATCH_SLOTS entries.
 */
static int watch_slot(const struct vattr_watch *frame, const v_attr_t *attr, vuint32_t *index)
{
    uintptr_t begin;
    uintptr_t at;
    uintptr_t span;

    if (frame->base == NULL || attr == NULL || frame->count == 0) {
        return 0;
    }
    begin = (uintptr_t)frame->base;
    at = (uintptr_t)attr;
    if (at < begin) {
        return 0;
    }
    if (frame->count > UINTPTR_MAX / sizeof(v_attr_t)) {
        return 0;
    }
    span = (uintptr_t)frame->count * sizeof(v_attr_t);
    if (at - begin >= span || (at - begin) % sizeof(v_attr_t) != 0) {
        return 0;
    }
    *index = (vuint32_t)((at - begin) / sizeof(v_attr_t));
    if (*index >= VATTR_WATCH_SLOTS) {
        return 0;
    }
    return 1;
}

/**
 * @brief Mark @p attr in the current watch when it lies in that range.
 *
 * @param[in] attr Attribute a writer has accepted
 */
static void note_write(const v_attr_t *attr)
{
    struct vattr_watch *frame;
    vuint32_t index;

    if (watch_depth == 0) {
        return;
    }
    frame = &watch_stack[watch_depth - 1];
    if (!watch_slot(frame, attr, &index)) {
        return;
    }
    frame->bits |= 1u << index;
}

/**
 * @brief Remember writer calls on this thread for @p count attributes at @p base.
 *
 * @param[in] base  First attribute of the range, or NULL
 * @param[in] count Number of attributes in the range
 */
void vattr_write_watch(v_attr_t *base, vuint32_t count)
{
    struct vattr_watch *frame;

    if (watch_depth >= VATTR_WATCH_DEPTH) {
        /* Keep the saved frames. The matching unwatch only drops this count. */
        watch_spare++;
        return;
    }
    frame = &watch_stack[watch_depth];
    frame->base = base;
    frame->count = base == NULL ? 0 : count;
    frame->bits = 0;
    watch_depth++;
}

/**
 * @brief Restore the write watch saved before the matching vattr_write_watch.
 */
void vattr_write_unwatch(void)
{
    if (watch_spare > 0) {
        watch_spare--;
        return;
    }
    if (watch_depth == 0) {
        return;
    }
    watch_depth--;
    watch_stack[watch_depth].base = NULL;
    watch_stack[watch_depth].count = 0;
    watch_stack[watch_depth].bits = 0;
}

/**
 * @brief Return non-zero when the current watch saw a writer accept @p attr.
 *
 * @param[in] attr Attribute to test, or NULL
 * @return Non-zero when a writer accepted @p attr under the current watch.
 */
int vattr_was_written(const v_attr_t *attr)
{
    vuint32_t index;

    if (watch_depth == 0) {
        return 0;
    }
    if (!watch_slot(&watch_stack[watch_depth - 1], attr, &index)) {
        return 0;
    }
    return (watch_stack[watch_depth - 1].bits & (1u << index)) != 0;
}

/**
 * @brief Write text into the buffer the caller gave in @p attr.
 *
 * @param[in,out] attr Attribute whose text buffer receives the bytes
 * @param[in]     src  Bytes to write
 * @param[in]     len  Number of bytes in @p src
 * @return V_STATUS_SUCCESS, V_STATUS_BUFFER_OVERFLOW when @p len exceeds the room,
 *         or V_STATUS_INVALID_PARAMETER when @p attr, or @p src with a length, is NULL.
 */
v_status_t vattr_put_text(v_attr_t *attr, const char *src, vuint32_t len)
{
    vuint32_t room;

    if (attr == NULL || (len > 0 && src == NULL)) {
        return V_STATUS_INVALID_PARAMETER;
    }
    note_write(attr);
    room = attr->value.text.len;
    attr->value.text.len = len;
    if (len > room) {
        return V_STATUS_BUFFER_OVERFLOW;
    }
    if (len == 0) {
        return V_STATUS_SUCCESS;
    }
    if (attr->value.text.buf == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    vplatform_memcpy(attr->value.text.buf, src, len);
    return V_STATUS_SUCCESS;
}

/**
 * @brief Write list items into the array the caller gave in @p attr.
 *
 * @param[in,out] attr      Attribute whose item array receives the items
 * @param[in]     items     Items to write, one after another
 * @param[in]     count     Number of items in @p items
 * @param[in]     elem_size Bytes in one item
 * @return V_STATUS_SUCCESS, V_STATUS_BUFFER_OVERFLOW when @p count exceeds the room,
 *         or V_STATUS_INVALID_PARAMETER when @p attr, or @p items with a count, is NULL.
 */
v_status_t vattr_put_list(v_attr_t *attr, const void *items, vuint32_t count, vuint32_t elem_size)
{
    vuint32_t room;

    if (attr == NULL || (count > 0 && items == NULL)) {
        return V_STATUS_INVALID_PARAMETER;
    }
    note_write(attr);
    room = attr->value.list.count;
    attr->value.list.count = count;
    if (count > room) {
        return V_STATUS_BUFFER_OVERFLOW;
    }
    if (count == 0 || elem_size == 0) {
        return V_STATUS_SUCCESS;
    }
    if (attr->value.list.items == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    vplatform_memcpy(attr->value.list.items, items, (size_t)count * elem_size);
    return V_STATUS_SUCCESS;
}

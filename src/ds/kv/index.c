/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "ds/index.h"

#include "platform/memory.h"

enum { VDS_INDEX_MIN = 8 };

/**
 * @brief Allocate an index store with room for @p capacity entries.
 *
 * @param[out] store    Receives the store
 * @param[in]  capacity Initial slot count. Zero selects 8.
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_index_create(vds_index_t **store, vuint32_t capacity)
{
    vds_index_t *index;
    vuint32_t n;

    if (store == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    n = capacity == 0 ? VDS_INDEX_MIN : capacity;
    index = vplatform_calloc(1, sizeof(*index));
    if (index == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    index->slot = vplatform_calloc(n, sizeof(*index->slot));
    if (index->slot == NULL) {
        vplatform_free(index);
        return V_STATUS_NO_MEMORY;
    }
    index->capacity = n;
    *store = index;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Free the store. Stored pointers are not freed.
 *
 * @param[in] store Store object
 */
void vds_index_destroy(vds_index_t *store)
{
    if (store == NULL) {
        return;
    }
    vplatform_free(store->slot);
    vplatform_free(store);
}

/**
 * @brief Read the pointer at @p index.
 *
 * @param[in]  store  Store object
 * @param[in]  index  Slot number
 * @param[out] object Receives the pointer
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_index_get(const vds_index_t *store, vuint32_t index, void **object)
{
    if (store == NULL || object == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (index >= store->capacity || store->slot[index] == NULL) {
        return V_STATUS_ITEM_NOT_FOUND;
    }
    *object = store->slot[index];
    return V_STATUS_SUCCESS;
}

/**
 * @brief Store @p object at @p index. The table grows when the index is past the end.
 *
 * @param[in,out] store  Store object
 * @param[in]     index  Slot number
 * @param[in]     object Caller-owned pointer. NULL is rejected.
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_index_set(vds_index_t *store, vuint32_t index, void *object)
{
    void **slot;
    vuint32_t capacity;

    if (store == NULL || object == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (index >= store->capacity) {
        capacity = store->capacity;
        while (capacity <= index) {
            if (capacity > 0x7fffffffu) {
                return V_STATUS_NO_MEMORY;
            }
            capacity <<= 1;
        }
        slot = vplatform_calloc(capacity, sizeof(*slot));
        if (slot == NULL) {
            return V_STATUS_NO_MEMORY;
        }
        vplatform_memcpy(slot, store->slot, sizeof(*slot) * store->capacity);
        vplatform_free(store->slot);
        store->slot = slot;
        store->capacity = capacity;
    }
    if (store->slot[index] == NULL) {
        store->count++;
    }
    store->slot[index] = object;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Clear the slot at @p index. The pointer is not freed.
 *
 * @param[in,out] store Store object
 * @param[in]     index Slot number
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_index_del(vds_index_t *store, vuint32_t index)
{
    if (store == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (index >= store->capacity || store->slot[index] == NULL) {
        return V_STATUS_ITEM_NOT_FOUND;
    }
    store->slot[index] = NULL;
    store->count--;
    return V_STATUS_SUCCESS;
}

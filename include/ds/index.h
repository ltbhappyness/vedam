/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VDS_INDEX_H
#define VDS_INDEX_H

#include "vi/types.h"

/**
 * @brief Dense pointer table. A NULL slot means the index is empty.
 */
typedef struct vds_index {
    void **slot;
    vuint32_t capacity;
    vuint32_t count;
} vds_index_t;

/**
 * @brief Allocate an index store with room for @p capacity entries.
 *
 * @param[out] store    Receives the store
 * @param[in]  capacity Initial slot count. Zero selects 8.
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_index_create(vds_index_t **store, vuint32_t capacity);

/**
 * @brief Free the store. Stored pointers are not freed.
 *
 * @param[in] store Store object
 */
void vds_index_destroy(vds_index_t *store);

/**
 * @brief Read the pointer at @p index.
 *
 * @param[in]  store  Store object
 * @param[in]  index  Slot number
 * @param[out] object Receives the pointer
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_index_get(const vds_index_t *store, vuint32_t index, void **object);

/**
 * @brief Store @p object at @p index. The table grows when the index is past the end.
 *
 * @param[in,out] store  Store object
 * @param[in]     index  Slot number
 * @param[in]     object Caller-owned pointer. NULL is rejected.
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_index_set(vds_index_t *store, vuint32_t index, void *object);

/**
 * @brief Clear the slot at @p index. The pointer is not freed.
 *
 * @param[in,out] store Store object
 * @param[in]     index Slot number
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_index_del(vds_index_t *store, vuint32_t index);

#endif

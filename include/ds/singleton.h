/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VDS_SINGLETON_H
#define VDS_SINGLETON_H

#include "vi/types.h"

/**
 * @brief One object slot. The object pointer is owned by the caller.
 */
typedef struct vds_singleton {
    void *object;
    int present;
} vds_singleton_t;

/**
 * @brief Allocate an empty singleton store.
 *
 * @param[out] store Receives the store
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_singleton_create(vds_singleton_t **store);

/**
 * @brief Free the store. The object pointer is not freed.
 *
 * @param[in] store Store object
 */
void vds_singleton_destroy(vds_singleton_t *store);

/**
 * @brief Read the stored object.
 *
 * @param[in]  store  Store object
 * @param[out] object Receives the pointer
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_singleton_get(const vds_singleton_t *store, void **object);

/**
 * @brief Store @p object. A previous object is replaced and not freed.
 *
 * @param[in,out] store  Store object
 * @param[in]     object Caller-owned pointer. NULL is rejected.
 * @return V_STATUS_SUCCESS or V_STATUS_INVALID_PARAMETER.
 */
v_status_t vds_singleton_set(vds_singleton_t *store, void *object);

/**
 * @brief Clear the slot. The object pointer is not freed.
 *
 * @param[in,out] store Store object
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_singleton_del(vds_singleton_t *store);

#endif

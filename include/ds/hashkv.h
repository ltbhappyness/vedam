/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VDS_HASHKV_H
#define VDS_HASHKV_H

#include "ds/hash.h"

/**
 * @brief Key-value store backed by the hash table.
 */
typedef struct vds_hashkv {
    vds_hash_t *table;
} vds_hashkv_t;

/**
 * @brief Allocate an empty key-value store.
 *
 * @param[out] store Receives the store
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_hashkv_create(vds_hashkv_t **store);

/**
 * @brief Free the store. Values are not freed.
 *
 * @param[in] store Store object
 */
void vds_hashkv_destroy(vds_hashkv_t *store);

/**
 * @brief Read the value stored under @p key.
 *
 * @param[in]  store   Store object
 * @param[in]  key     Key bytes
 * @param[in]  key_len Key length
 * @param[out] value   Receives the pointer
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_hashkv_get(vds_hashkv_t *store, const void *key, vuint32_t key_len, void **value);

/**
 * @brief Store @p value under @p key.
 *
 * @param[in,out] store   Store object
 * @param[in]     key     Key bytes
 * @param[in]     key_len Key length
 * @param[in]     value   Caller-owned pointer
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_hashkv_set(vds_hashkv_t *store, const void *key, vuint32_t key_len, void *value);

/**
 * @brief Remove @p key. The value pointer is not freed.
 *
 * @param[in,out] store   Store object
 * @param[in]     key     Key bytes
 * @param[in]     key_len Key length
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_hashkv_del(vds_hashkv_t *store, const void *key, vuint32_t key_len);

#endif

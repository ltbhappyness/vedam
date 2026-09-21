/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VDS_HASH_H
#define VDS_HASH_H

#include "ds/list.h"

/**
 * @brief One bucket chain node. The key bytes follow the struct.
 */
struct vds_hash_node {
    struct vds_list link;
    vuint32_t hash;
    vuint32_t key_len;
    void *value;
    unsigned char key[];
};

/**
 * @brief Hash table of caller-owned values. Keys are copied into each node.
 */
typedef struct vds_hash {
    struct vds_list *bucket;
    vuint32_t nbucket;
    vuint32_t count;
} vds_hash_t;

/**
 * @brief Hash @p key with FNV-1a.
 *
 * @param[in] key Key bytes
 * @param[in] len Key length
 * @return Hash value.
 */
vuint32_t vds_hash_bytes(const void *key, vuint32_t len);

/**
 * @brief Allocate a table with @p nbucket buckets. The count is rounded up to a power of two.
 *
 * @param[out] table    Receives the table
 * @param[in]  nbucket  Requested bucket count
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_hash_create(vds_hash_t **table, vuint32_t nbucket);

/**
 * @brief Free the table and its nodes. Values are left to the caller.
 *
 * @param[in] table Table object
 */
void vds_hash_destroy(vds_hash_t *table);

/**
 * @brief Store @p value under a copy of @p key. A previous value for the same key is replaced.
 *
 * @param[in,out] table Table object
 * @param[in]     key   Key bytes
 * @param[in]     key_len Key length
 * @param[in]     value Caller-owned value. NULL is rejected.
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_hash_set(vds_hash_t *table, const void *key, vuint32_t key_len, void *value);

/**
 * @brief Look up @p key.
 *
 * @param[in]  table Table object
 * @param[in]  key   Key bytes
 * @param[in]  key_len Key length
 * @param[out] value Receives the stored pointer
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_hash_get(vds_hash_t *table, const void *key, vuint32_t key_len, void **value);

/**
 * @brief Remove @p key. The value pointer is not freed.
 *
 * @param[in,out] table Table object
 * @param[in]     key   Key bytes
 * @param[in]     key_len Key length
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_hash_del(vds_hash_t *table, const void *key, vuint32_t key_len);

/**
 * @brief Allocate a node that carries a copy of @p key. The caller frees it if it is not inserted.
 */
struct vds_hash_node *vds_hash_node_make(const void *key, vuint32_t key_len, void *value);

/**
 * @brief Link @p node. Does not allocate. V_STATUS_NO_MEMORY means the table needs a larger bucket array.
 * V_STATUS_ITEM_ALREADY_EXISTS leaves @p node unlinked.
 */
v_status_t vds_hash_insert(vds_hash_t *table, struct vds_hash_node *node);

/**
 * @brief Allocate @p nbucket empty lists. @p nbucket must be a power of two.
 */
struct vds_list *vds_hash_prepare(vuint32_t nbucket);

/**
 * @brief Free a bucket array whose chains are empty.
 */
void vds_hash_buckets_free(struct vds_list *bucket);

/**
 * @brief Install @p fresh under the caller's lock. Returns the previous array.
 */
struct vds_list *vds_hash_swap(vds_hash_t *table, struct vds_list *fresh, vuint32_t nbucket,
                               vuint32_t *old_n);

#endif

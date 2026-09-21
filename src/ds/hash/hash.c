/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "ds/hash.h"

#include "platform/memory.h"

enum { VDS_HASH_MIN_BUCKETS = 8 };

/**
 * @brief Hash @p key with FNV-1a.
 *
 * @param[in] key Key bytes
 * @param[in] len Key length
 * @return Hash value.
 */
vuint32_t vds_hash_bytes(const void *key, vuint32_t len)
{
    const unsigned char *bytes = (const unsigned char *)key;
    vuint32_t hash = 2166136261u;
    vuint32_t i;

    if (bytes == NULL) {
        return 0;
    }
    for (i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

/**
 * @brief Round @p n up to the next power of two.
 * @param[in] value Argument value
 * @return Result of round_pow2.
 */
static vuint32_t round_pow2(vuint32_t value)
{
    vuint32_t n = VDS_HASH_MIN_BUCKETS;

    if (value < VDS_HASH_MIN_BUCKETS) {
        return VDS_HASH_MIN_BUCKETS;
    }
    while (n < value && n < 0x80000000u) {
        n <<= 1;
    }
    return n;
}

/**
 * @brief Return non-zero when two hash keys are equal.
 * @param[in] node Argument node
 * @param[in] key Argument key
 * @param[in] key_len Argument key_len
 * @param[in] hash Argument hash
 * @return Result of key_equal.
 */
static int key_equal(const struct vds_hash_node *node, const void *key, vuint32_t key_len,
                     vuint32_t hash)
{
    vuint32_t i;

    if (node->hash != hash || node->key_len != key_len) {
        return 0;
    }
    for (i = 0; i < key_len; i++) {
        if (node->key[i] != ((const unsigned char *)key)[i]) {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Find a hash node for @p key.
 * @param[in] table Argument table
 * @param[in] key Argument key
 * @param[in] key_len Argument key_len
 * @param[in] hash Argument hash
 * @return Result of find_node.
 */
static struct vds_hash_node *find_node(vds_hash_t *table, const void *key, vuint32_t key_len,
                                       vuint32_t hash)
{
    struct vds_list *head = &table->bucket[hash & (table->nbucket - 1)];
    struct vds_list *pos;

/**
 * @brief Walk each element in a circular list.
 * @param[in] pos Argument pos
 * @param[in] head Argument head
 * @return Result of vds_list_for_each.
 */
    vds_list_for_each(pos, head) {
        struct vds_hash_node *node = vds_container_of(pos, struct vds_hash_node, link);

        if (key_equal(node, key, key_len, hash)) {
            return node;
        }
    }
    return NULL;
}

/**
 * @brief Rebuild the hash table into a larger bucket array.
 * @param[in] table Argument table
 * @param[in] nbucket Argument nbucket
 * @return Result of rehash.
 */
static v_status_t rehash(vds_hash_t *table, vuint32_t nbucket)
{
    struct vds_list *bucket;
    struct vds_list *old;
    vuint32_t old_n;
    vuint32_t i;

    bucket = (struct vds_list *)vplatform_calloc(nbucket, sizeof(*bucket));
    if (bucket == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    for (i = 0; i < nbucket; i++) {
        vds_list_init(&bucket[i]);
    }
    old = table->bucket;
    old_n = table->nbucket;
    table->bucket = bucket;
    table->nbucket = nbucket;
    for (i = 0; i < old_n; i++) {
        struct vds_list *pos;
        struct vds_list *tmp;

/**
 * @brief Walk each element in a circular list while allowing removal.
 * @param[in] pos Argument pos
 * @param[in] tmp Argument tmp
 * @param[in] old[i] Argument old[i]
 * @return Result of vds_list_for_each_safe.
 */
        vds_list_for_each_safe(pos, tmp, &old[i]) {
            struct vds_hash_node *node = vds_container_of(pos, struct vds_hash_node, link);

            vds_list_del(pos);
            vds_list_add(&node->link, &table->bucket[node->hash & (nbucket - 1)]);
        }
    }
    vplatform_free(old);
    return V_STATUS_SUCCESS;
}

/**
 * @brief Allocate a table with @p nbucket buckets. The count is rounded up to a power of two.
 *
 * @param[out] table   Receives the table
 * @param[in]  nbucket Requested bucket count
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_hash_create(vds_hash_t **table, vuint32_t nbucket)
{
    vds_hash_t *hash;
    vuint32_t n;
    vuint32_t i;

    if (table == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    n = round_pow2(nbucket);
    hash = (vds_hash_t *)vplatform_calloc(1, sizeof(*hash));
    if (hash == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    hash->bucket = (struct vds_list *)vplatform_calloc(n, sizeof(*hash->bucket));
    if (hash->bucket == NULL) {
        vplatform_free(hash);
        return V_STATUS_NO_MEMORY;
    }
    for (i = 0; i < n; i++) {
        vds_list_init(&hash->bucket[i]);
    }
    hash->nbucket = n;
    *table = hash;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Free the table and its nodes. Values are left to the caller.
 *
 * @param[in] table Table object
 */
void vds_hash_destroy(vds_hash_t *table)
{
    vuint32_t i;

    if (table == NULL) {
        return;
    }
    for (i = 0; i < table->nbucket; i++) {
        struct vds_list *pos;
        struct vds_list *tmp;

/**
 * @brief Walk each element in a circular list while allowing removal.
 * @param[in] pos Argument pos
 * @param[in] tmp Argument tmp
 * @param[in] table->bucket[i] Argument table->bucket[i]
 * @return Result of vds_list_for_each_safe.
 */
        vds_list_for_each_safe(pos, tmp, &table->bucket[i]) {
            struct vds_hash_node *node = vds_container_of(pos, struct vds_hash_node, link);

            vds_list_del(pos);
            vplatform_free(node);
        }
    }
    vplatform_free(table->bucket);
    vplatform_free(table);
}

/**
 * @brief Store @p value under a copy of @p key. A previous value for the same key is replaced.
 *
 * @param[in,out] table   Table object
 * @param[in]     key     Key bytes
 * @param[in]     key_len Key length
 * @param[in]     value   Caller-owned value. NULL is rejected.
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_hash_set(vds_hash_t *table, const void *key, vuint32_t key_len, void *value)
{
    struct vds_hash_node *node;
    vuint32_t hash;
    v_status_t rc;

    if (table == NULL || key == NULL || key_len == 0 || value == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (sizeof(*node) + (size_t)key_len < (size_t)key_len) {
        return V_STATUS_INVALID_PARAMETER;
    }
    hash = vds_hash_bytes(key, key_len);
    node = find_node(table, key, key_len, hash);
    if (node != NULL) {
        node->value = value;
        return V_STATUS_SUCCESS;
    }
    if (table->count + 1 > table->nbucket) {
        rc = rehash(table, table->nbucket << 1);
        if (rc != V_STATUS_SUCCESS) {
            return rc;
        }
    }
    node = (struct vds_hash_node *)vplatform_calloc(1, sizeof(*node) + key_len);
    if (node == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    node->hash = hash;
    node->key_len = key_len;
    node->value = value;
    vplatform_memcpy(node->key, key, key_len);
    vds_list_add(&node->link, &table->bucket[hash & (table->nbucket - 1)]);
    table->count++;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Look up @p key.
 *
 * @param[in]  table   Table object
 * @param[in]  key     Key bytes
 * @param[in]  key_len Key length
 * @param[out] value   Receives the stored pointer
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_hash_get(vds_hash_t *table, const void *key, vuint32_t key_len, void **value)
{
    struct vds_hash_node *node;

    if (table == NULL || key == NULL || key_len == 0 || value == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    node = find_node(table, key, key_len, vds_hash_bytes(key, key_len));
    if (node == NULL) {
        return V_STATUS_ITEM_NOT_FOUND;
    }
    *value = node->value;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Remove @p key. The value pointer is not freed.
 *
 * @param[in,out] table   Table object
 * @param[in]     key     Key bytes
 * @param[in]     key_len Key length
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_hash_del(vds_hash_t *table, const void *key, vuint32_t key_len)
{
    struct vds_hash_node *node;

    if (table == NULL || key == NULL || key_len == 0) {
        return V_STATUS_INVALID_PARAMETER;
    }
    node = find_node(table, key, key_len, vds_hash_bytes(key, key_len));
    if (node == NULL) {
        return V_STATUS_ITEM_NOT_FOUND;
    }
    vds_list_del(&node->link);
    vplatform_free(node);
    table->count--;
    return V_STATUS_SUCCESS;
}

struct vds_hash_node *vds_hash_node_make(const void *key, vuint32_t key_len, void *value)
{
    struct vds_hash_node *node;

    if (key == NULL || key_len == 0 || value == NULL) {
        return NULL;
    }
    if (sizeof(*node) + (size_t)key_len < (size_t)key_len) {
        return NULL;
    }
    node = (struct vds_hash_node *)vplatform_calloc(1, sizeof(*node) + key_len);
    if (node == NULL) {
        return NULL;
    }
    node->hash = vds_hash_bytes(key, key_len);
    node->key_len = key_len;
    node->value = value;
    vplatform_memcpy(node->key, key, key_len);
    return node;
}

/**
 * @brief Insert a node into the hash table.
 * @param[in] table Argument table
 * @param[in] node Argument node
 * @return Result of vds_hash_insert.
 */
v_status_t vds_hash_insert(vds_hash_t *table, struct vds_hash_node *node)
{
    if (table == NULL || node == NULL || node->key_len == 0) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (table->count + 1 > table->nbucket) {
        return V_STATUS_NO_MEMORY;
    }
    if (find_node(table, node->key, node->key_len, node->hash) != NULL) {
        return V_STATUS_ITEM_ALREADY_EXISTS;
    }
    vds_list_add(&node->link, &table->bucket[node->hash & (table->nbucket - 1)]);
    table->count++;
    return V_STATUS_SUCCESS;
}

struct vds_list *vds_hash_prepare(vuint32_t nbucket)
{
    struct vds_list *bucket;
    vuint32_t i;

    if (nbucket == 0 || (nbucket & (nbucket - 1u)) != 0) {
        return NULL;
    }
    bucket = (struct vds_list *)vplatform_calloc(nbucket, sizeof(*bucket));
    if (bucket == NULL) {
        return NULL;
    }
    for (i = 0; i < nbucket; i++) {
        vds_list_init(&bucket[i]);
    }
    return bucket;
}

/**
 * @brief Free every bucket chain in the hash table.
 * @param[in] bucket Argument bucket
 */
void vds_hash_buckets_free(struct vds_list *bucket)
{
    vplatform_free(bucket);
}

struct vds_list *vds_hash_swap(vds_hash_t *table, struct vds_list *fresh, vuint32_t nbucket,
                               vuint32_t *old_n)
{
    struct vds_list *old;
    vuint32_t old_count;
    vuint32_t i;

    if (table == NULL || fresh == NULL || old_n == NULL || nbucket == 0) {
        return NULL;
    }
    old = table->bucket;
    old_count = table->nbucket;
    table->bucket = fresh;
    table->nbucket = nbucket;
    for (i = 0; i < old_count; i++) {
        struct vds_list *pos;
        struct vds_list *tmp;

/**
 * @brief Walk each element in a circular list while allowing removal.
 * @param[in] pos Argument pos
 * @param[in] tmp Argument tmp
 * @param[in] old[i] Argument old[i]
 * @return Result of vds_list_for_each_safe.
 */
        vds_list_for_each_safe(pos, tmp, &old[i]) {
            struct vds_hash_node *node = vds_container_of(pos, struct vds_hash_node, link);

            vds_list_del(pos);
            vds_list_add(&node->link, &table->bucket[node->hash & (nbucket - 1)]);
        }
    }
    *old_n = old_count;
    return old;
}

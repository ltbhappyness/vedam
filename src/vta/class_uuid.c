/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#include "vti/class_uuid.h"

#include "core/node.h"
#include "ds/hash.h"
#include "platform/memory.h"
#include "vii/class/identity.h"
#include "vii/class/security.h"
#include "vii/class/storage.h"
#include "vii/class/system.h"
#include "vii/class/wasm.h"

#include <string.h>

/**
 * @brief One hash value: class index plus the UUID line major and minor.
 */
struct vta_uuid_hit {
    v_class_id_t class_id;
    vuint32_t major;
    vuint32_t minor;
};

/**
 * @brief One seed row used to fill the UUID hash from the vii tables.
 */
struct vta_uuid_seed {
    const uint8_t *uuid;
    v_class_id_t class_id;
    vuint32_t major;
    vuint32_t minor;
};

/**
 * @brief One seed row used to fill the service hash.
 */
struct vta_service_seed {
    const char *name;
    vuint32_t name_len;
    v_class_id_t class_id;
    vuint32_t major;
    vuint32_t minor;
};

/**
 * @brief Service names for one class, indexed by major.
 */
struct vta_service_names {
    const char *const *name;
    vuint32_t count;
};

static const struct vta_uuid_seed g_uuid_seeds[] = {
    {vii_identity_uuid[1][0], V_CLASS_IDENTITY, 1, 0},
    {vii_security_uuid[1][0], V_CLASS_SECURITY, 1, 0},
    {vii_storage_uuid[1][0], V_CLASS_STORAGE, 1, 0},
    {vii_system_uuid[1][0], V_CLASS_SYSTEM, 1, 0},
    {vii_wasm_uuid[1][0], V_CLASS_WASM, 1, 0},
};

enum { VTA_UUID_SEED_COUNT = (int)(sizeof(g_uuid_seeds) / sizeof(g_uuid_seeds[0])) };

static const struct vta_service_seed g_service_seeds[] = {
    {"vedam.identity.v1.IdentityService", 33, V_CLASS_IDENTITY, 1, 0},
    {"vedam.security.v1.SecurityService", 33, V_CLASS_SECURITY, 1, 0},
    {"vedam.storage.v1.StorageService", 31, V_CLASS_STORAGE, 1, 0},
    {"vedam.system.v1.SystemService", 29, V_CLASS_SYSTEM, 1, 0},
    {"vedam.wasm.v1.WasmService", 25, V_CLASS_WASM, 1, 0},
};

enum { VTA_SERVICE_SEED_COUNT = (int)(sizeof(g_service_seeds) / sizeof(g_service_seeds[0])) };

static const char *const identity_services[VII_IDENTITY_UUID_MAJOR_MAX + 1] = {
    [1] = "vedam.identity.v1.IdentityService",
};

static const char *const security_services[VII_SECURITY_UUID_MAJOR_MAX + 1] = {
    [1] = "vedam.security.v1.SecurityService",
};

static const char *const storage_services[VII_STORAGE_UUID_MAJOR_MAX + 1] = {
    [1] = "vedam.storage.v1.StorageService",
};

static const char *const system_services[VII_SYSTEM_UUID_MAJOR_MAX + 1] = {
    [1] = "vedam.system.v1.SystemService",
};

static const char *const wasm_services[VII_WASM_UUID_MAJOR_MAX + 1] = {
    [1] = "vedam.wasm.v1.WasmService",
};

static const struct vta_service_names g_service_names[V_CLASS_ID_MAX + 1] = {
    [V_CLASS_IDENTITY] = {identity_services, VII_IDENTITY_UUID_MAJOR_MAX + 1},
    [V_CLASS_SECURITY] = {security_services, VII_SECURITY_UUID_MAJOR_MAX + 1},
    [V_CLASS_STORAGE] = {storage_services, VII_STORAGE_UUID_MAJOR_MAX + 1},
    [V_CLASS_SYSTEM] = {system_services, VII_SYSTEM_UUID_MAJOR_MAX + 1},
    [V_CLASS_WASM] = {wasm_services, VII_WASM_UUID_MAJOR_MAX + 1},
};

static const uint8_t *const g_latest_uuid[V_CLASS_ID_MAX + 1] = {
    [V_CLASS_IDENTITY] = vii_identity_uuid[VII_IDENTITY_MAJOR][VII_IDENTITY_MINOR],
    [V_CLASS_SECURITY] = vii_security_uuid[VII_SECURITY_MAJOR][VII_SECURITY_MINOR],
    [V_CLASS_STORAGE] = vii_storage_uuid[VII_STORAGE_MAJOR][VII_STORAGE_MINOR],
    [V_CLASS_SYSTEM] = vii_system_uuid[VII_SYSTEM_MAJOR][VII_SYSTEM_MINOR],
    [V_CLASS_WASM] = vii_wasm_uuid[VII_WASM_MAJOR][VII_WASM_MINOR],
};

/**
 * @brief Build the UUID hash from every supported vii table line, then the service hash.
 *
 * Hit records are allocated with the map on the node. The seed table stays
 * static const. g_vedam_node remains the only process-wide mutable global.
 *
 * @return V_STATUS_SUCCESS, V_STATUS_UNINITIALIZED, or V_STATUS_NO_MEMORY.
 */
v_status_t vti_class_uuid_map_init(void)
{
    struct vta_uuid_hit *hits;
    vedam_t *node;
    vds_hash_t *map;
    v_status_t st;
    int i;

    node = vedam_node();
    if (node == NULL) {
        return V_STATUS_UNINITIALIZED;
    }
    if (node->uuid_map != NULL) {
        return V_STATUS_SUCCESS;
    }

    hits = vplatform_calloc((size_t)VTA_UUID_SEED_COUNT, sizeof(*hits));
    if (hits == NULL) {
        return V_STATUS_NO_MEMORY;
    }

    st = vds_hash_create(&map, 16);
    if (st != V_STATUS_SUCCESS) {
        goto out_free_hits;
    }

    for (i = 0; i < VTA_UUID_SEED_COUNT; i++) {
        hits[i].class_id = g_uuid_seeds[i].class_id;
        hits[i].major = g_uuid_seeds[i].major;
        hits[i].minor = g_uuid_seeds[i].minor;
        st = vds_hash_set(map, g_uuid_seeds[i].uuid, VTI_CLASS_UUID_LEN, &hits[i]);
        if (st != V_STATUS_SUCCESS) {
            goto out_destroy_map;
        }
    }
    node->uuid_hits = hits;
    node->uuid_map = map;

    st = vti_service_map_init();
    if (st != V_STATUS_SUCCESS) {
        node->uuid_hits = NULL;
        node->uuid_map = NULL;
        goto out_destroy_map;
    }
    return V_STATUS_SUCCESS;

out_destroy_map:
    vds_hash_destroy(map);
out_free_hits:
    vplatform_free(hits);
    return st;
}

/**
 * @brief Free the maps and hit records built by vti_class_uuid_map_init.
 */
void vti_class_uuid_map_fini(void)
{
    vedam_t *node;

    node = vedam_node();
    if (node == NULL) {
        return;
    }
    vti_service_map_fini();
    if (node->uuid_map != NULL) {
        vds_hash_destroy(node->uuid_map);
        node->uuid_map = NULL;
    }
    if (node->uuid_hits != NULL) {
        vplatform_free(node->uuid_hits);
        node->uuid_hits = NULL;
    }
}

/**
 * @brief Build the service hash that maps a gRPC service name to class, major, and minor.
 *
 * @return V_STATUS_SUCCESS, V_STATUS_UNINITIALIZED, or V_STATUS_NO_MEMORY.
 */
v_status_t vti_service_map_init(void)
{
    struct vta_uuid_hit *hits;
    vedam_t *node;
    vds_hash_t *map;
    v_status_t st;
    int i;

    node = vedam_node();
    if (node == NULL) {
        return V_STATUS_UNINITIALIZED;
    }
    if (node->service_map != NULL) {
        return V_STATUS_SUCCESS;
    }

    hits = vplatform_calloc((size_t)VTA_SERVICE_SEED_COUNT, sizeof(*hits));
    if (hits == NULL) {
        return V_STATUS_NO_MEMORY;
    }

    st = vds_hash_create(&map, 16);
    if (st != V_STATUS_SUCCESS) {
        goto out_free_hits;
    }

    for (i = 0; i < VTA_SERVICE_SEED_COUNT; i++) {
        hits[i].class_id = g_service_seeds[i].class_id;
        hits[i].major = g_service_seeds[i].major;
        hits[i].minor = g_service_seeds[i].minor;
        st = vds_hash_set(map, g_service_seeds[i].name, g_service_seeds[i].name_len, &hits[i]);
        if (st != V_STATUS_SUCCESS) {
            goto out_destroy_map;
        }
    }
    node->service_hits = hits;
    node->service_map = map;
    return V_STATUS_SUCCESS;

out_destroy_map:
    vds_hash_destroy(map);
out_free_hits:
    vplatform_free(hits);
    return st;
}

/**
 * @brief Free the service hash and its hit records.
 */
void vti_service_map_fini(void)
{
    vedam_t *node;

    node = vedam_node();
    if (node == NULL) {
        return;
    }
    if (node->service_map != NULL) {
        vds_hash_destroy(node->service_map);
        node->service_map = NULL;
    }
    if (node->service_hits != NULL) {
        vplatform_free(node->service_hits);
        node->service_hits = NULL;
    }
}

/**
 * @brief Copy one hash hit into the caller outputs.
 *
 * @param[in]  hit       Hash value
 * @param[out] out_class Class index
 * @param[out] out_major Vertical major
 * @param[out] out_minor Vertical minor
 */
static void copy_hit(const struct vta_uuid_hit *hit, v_class_id_t *out_class, vuint32_t *out_major,
                     vuint32_t *out_minor)
{
    *out_class = hit->class_id;
    *out_major = hit->major;
    *out_minor = hit->minor;
}

/**
 * @brief Map a 16-byte vertical UUID to class index, major, and minor.
 *
 * @param[in]  uuid      Sixteen UUID bytes
 * @param[out] out_class Class index on success
 * @param[out] out_major Vertical major on success
 * @param[out] out_minor Vertical minor on success
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, V_STATUS_UNINITIALIZED,
 *         or V_STATUS_NOT_IMPLEMENTED.
 */
v_status_t vti_class_from_uuid(const uint8_t *uuid, v_class_id_t *out_class, vuint32_t *out_major,
                               vuint32_t *out_minor)
{
    vedam_t *node;
    void *value;
    v_status_t st;

    if (uuid == NULL || out_class == NULL || out_major == NULL || out_minor == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }

    node = vedam_node();
    if (node == NULL || node->uuid_map == NULL) {
        return V_STATUS_UNINITIALIZED;
    }

    st = vds_hash_get(node->uuid_map, uuid, VTI_CLASS_UUID_LEN, &value);
    if (st != V_STATUS_SUCCESS) {
        return V_STATUS_NOT_IMPLEMENTED;
    }
    copy_hit(value, out_class, out_major, out_minor);
    return V_STATUS_SUCCESS;
}

/**
 * @brief Map a gRPC service name to class index, major, and the latest minor of that major.
 *
 * @param[in]  service   NUL-terminated service name
 * @param[out] out_class Class index on success
 * @param[out] out_major Vertical major on success
 * @param[out] out_minor Vertical minor on success
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, V_STATUS_UNINITIALIZED,
 *         or V_STATUS_NOT_IMPLEMENTED.
 */
v_status_t vti_service_lookup(const char *service, v_class_id_t *out_class, vuint32_t *out_major,
                              vuint32_t *out_minor)
{
    vedam_t *node;
    void *value;
    v_status_t st;
    size_t len;

    if (service == NULL || out_class == NULL || out_major == NULL || out_minor == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }

    node = vedam_node();
    if (node == NULL || node->service_map == NULL) {
        return V_STATUS_UNINITIALIZED;
    }

    len = strlen(service);
    st = vds_hash_get(node->service_map, service, (vuint32_t)len, &value);
    if (st != V_STATUS_SUCCESS) {
        return V_STATUS_NOT_IMPLEMENTED;
    }
    copy_hit(value, out_class, out_major, out_minor);
    return V_STATUS_SUCCESS;
}

/**
 * @brief Return the gRPC service name for @p class_id at @p major, or NULL when unknown.
 *
 * @param[in] class_id Local class index
 * @param[in] major    Vertical major
 * @return Static NUL-terminated service name, or NULL.
 */
const char *vti_service_name(v_class_id_t class_id, vuint32_t major)
{
    const struct vta_service_names *names;

    if ((unsigned)class_id > (unsigned)V_CLASS_ID_MAX) {
        return NULL;
    }
    names = &g_service_names[class_id];
    if (major >= names->count) {
        return NULL;
    }
    return names->name[major];
}

/**
 * @brief Return the latest 16-byte UUID for @p class_id, or NULL when unknown.
 *
 * @param[in] class_id Local class index
 * @return Pointer to sixteen bytes, or NULL.
 */
const uint8_t *vti_class_uuid_bytes(v_class_id_t class_id)
{
    if ((unsigned)class_id > (unsigned)V_CLASS_ID_MAX) {
        return NULL;
    }
    return g_latest_uuid[class_id];
}

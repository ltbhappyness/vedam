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
#include "vii/class/identity.h"
#include "vii/class/security.h"
#include "vii/class/storage.h"
#include "vii/class/system.h"
#include "vii/class/wasm.h"

#include "core/core.h"
#include "ds/hash.h"
#include "platform/memory.h"

#include <stddef.h>
#include <stdio.h>

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

static const struct vta_uuid_seed g_uuid_seeds[] = {
    { vii_identity_uuid[1][0], V_CLASS_IDENTITY,
      1, 0 },
    { vii_security_uuid[1][0], V_CLASS_SECURITY,
      1, 0 },
    { vii_storage_uuid[1][0], V_CLASS_STORAGE,
      1, 0 },
    { vii_system_uuid[1][0], V_CLASS_SYSTEM,
      1, 0 },
    { vii_wasm_uuid[1][0], V_CLASS_WASM,
      1, 0 },
};

enum { VTA_UUID_SEED_COUNT = (int)(sizeof(g_uuid_seeds) / sizeof(g_uuid_seeds[0])) };

/**
 * @brief Convert one hex character to its nibble, or -1 when invalid.
 *
 * @param[in] c Character to convert
 * @return Nibble value 0..15, or -1.
 */
static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/**
 * @brief Build the UUID hash from every supported vii table line.
 *
 * Hit records are allocated with the map on the node. The seed table stays
 * static const. vedam_node() remains the only process-wide mutable global.
 *
 * @return V_STATUS_SUCCESS, V_STATUS_UNINITIALIZED, or V_STATUS_NO_MEMORY.
 */
v_status_t vta_class_uuid_map_init(void)
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

    hits = (struct vta_uuid_hit *)vplatform_calloc((size_t)VTA_UUID_SEED_COUNT, sizeof(*hits));
    if (hits == NULL) {
        return V_STATUS_NO_MEMORY;
    }

    st = vds_hash_create(&map, 16);
    if (st != V_STATUS_SUCCESS) {
        vplatform_free(hits);
        return st;
    }

    for (i = 0; i < VTA_UUID_SEED_COUNT; i++) {
        hits[i].class_id = g_uuid_seeds[i].class_id;
        hits[i].major = g_uuid_seeds[i].major;
        hits[i].minor = g_uuid_seeds[i].minor;
        st = vds_hash_set(map, g_uuid_seeds[i].uuid, VTA_CLASS_UUID_LEN, &hits[i]);
        if (st != V_STATUS_SUCCESS) {
            vds_hash_destroy(map);
            vplatform_free(hits);
            return st;
        }
    }
    node->uuid_hits = hits;
    node->uuid_map = map;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Free the UUID hash and hit records built by vta_class_uuid_map_init.
 */
void vta_class_uuid_map_fini(void)
{
    vedam_t *node;

    node = vedam_node();
    if (node == NULL) {
        return;
    }
    if (node->uuid_map != NULL) {
        vds_hash_destroy((vds_hash_t *)node->uuid_map);
        node->uuid_map = NULL;
    }
    if (node->uuid_hits != NULL) {
        vplatform_free(node->uuid_hits);
        node->uuid_hits = NULL;
    }
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
v_status_t vta_class_from_uuid(const uint8_t *uuid, v_class_id_t *out_class, vuint32_t *out_major,
                               vuint32_t *out_minor)
{
    struct vta_uuid_hit *hit;
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

    st = vds_hash_get((vds_hash_t *)node->uuid_map, uuid, VTA_CLASS_UUID_LEN, &value);
    if (st != V_STATUS_SUCCESS) {
        return V_STATUS_NOT_IMPLEMENTED;
    }

    hit = (struct vta_uuid_hit *)value;
    *out_class = hit->class_id;
    *out_major = hit->major;
    *out_minor = hit->minor;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Return the latest 16-byte UUID for @p class_id, or NULL when unknown.
 *
 * @param[in] class_id Local class index
 * @return Pointer to sixteen bytes, or NULL.
 */
const uint8_t *vta_class_uuid_bytes(v_class_id_t class_id)
{
    switch (class_id) {
    case V_CLASS_IDENTITY:
        return vii_identity_uuid[VII_IDENTITY_MAJOR]
                                            [VII_IDENTITY_MINOR];
    case V_CLASS_SECURITY:
        return vii_security_uuid[VII_SECURITY_MAJOR]
                                            [VII_SECURITY_MINOR];
    case V_CLASS_STORAGE:
        return vii_storage_uuid[VII_STORAGE_MAJOR]
                                            [VII_STORAGE_MINOR];
    case V_CLASS_SYSTEM:
        return vii_system_uuid[VII_SYSTEM_MAJOR]
                                            [VII_SYSTEM_MINOR];
    case V_CLASS_WASM:
        return vii_wasm_uuid[VII_WASM_MAJOR]
                                            [VII_WASM_MINOR];
    default:
        return NULL;
    }
}

/**
 * @brief Format the latest UUID string for @p class_id into @p out.
 *
 * @param[in]  class_id Local class index
 * @param[out] out      Buffer for the 36-character UUID string plus NUL
 * @param[in]  out_len  Capacity of @p out
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NOT_IMPLEMENTED.
 */
v_status_t vta_class_uuid_string(v_class_id_t class_id, char *out, size_t out_len)
{
    const char *str;

    if (out == NULL || out_len < 37) {
        return V_STATUS_INVALID_PARAMETER;
    }
    switch (class_id) {
    case V_CLASS_IDENTITY:
        str = "29eb312d-057d-5930-bc70-bc6b01cb45ab";
        break;
    case V_CLASS_SECURITY:
        str = "3d8d5623-8ca8-585f-a434-664c1df38ffa";
        break;
    case V_CLASS_STORAGE:
        str = "77145e11-a47c-50da-bc42-a91ad8d764b8";
        break;
    case V_CLASS_SYSTEM:
        str = "c2b91580-6092-5c64-bd0c-ea7f377b56c1";
        break;
    case V_CLASS_WASM:
        str = "053ef9ba-7354-5891-ad47-7a529f4c1b64";
        break;
    default:
        return V_STATUS_NOT_IMPLEMENTED;
    }
    (void)snprintf(out, out_len, "%s", str);
    return V_STATUS_SUCCESS;
}

/**
 * @brief Parse a hyphenated UUID string into sixteen bytes.
 *
 * @param[in]  str Hyphenated UUID text
 * @param[out] out Sixteen-byte buffer
 * @return V_STATUS_SUCCESS or V_STATUS_INVALID_PARAMETER.
 */
v_status_t vta_class_uuid_parse(const char *str, uint8_t *out)
{
    static const int groups[5] = { 8, 4, 4, 4, 12 };
    int gi;
    int bi;
    int i;
    int hi;
    int lo;

    if (str == NULL || out == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }

    bi = 0;
    for (gi = 0; gi < 5; gi++) {
        if (gi > 0) {
            if (str[0] != '-') {
                return V_STATUS_INVALID_PARAMETER;
            }
            str++;
        }
        for (i = 0; i < groups[gi]; i += 2) {
            hi = hex_nibble(str[0]);
            lo = hex_nibble(str[1]);
            if (hi < 0 || lo < 0) {
                return V_STATUS_INVALID_PARAMETER;
            }
            out[bi++] = (uint8_t)((hi << 4) | lo);
            str += 2;
        }
    }
    if (str[0] != '\0') {
        return V_STATUS_INVALID_PARAMETER;
    }
    return V_STATUS_SUCCESS;
}

/**
 * @brief Non-zero when @p class_id is a singleton vertical.
 *
 * @param[in] class_id Local class index
 * @return 1 for singleton, 0 for keyed, -1 when the class is unknown.
 */
int vedam_lock_class_singleton(v_class_id_t class_id)
{
    switch (class_id) {
    case V_CLASS_IDENTITY:
        return 0;
    case V_CLASS_SECURITY:
        return 1;
    case V_CLASS_STORAGE:
        return 0;
    case V_CLASS_SYSTEM:
        return 1;
    case V_CLASS_WASM:
        return 1;
    default:
        return -1;
    }
}

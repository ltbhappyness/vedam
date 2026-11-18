/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * Gets across the verticals, UUID table checks, and lock miss counting.
 */
#include "core/node.h"
#include "lock/store.h"
#include "vi/class/contract.h"
#include "vi/class/identity.h"
#include "vi/class/plugin.h"
#include "vi/class/security.h"
#include "vi/class/storage.h"
#include "vi/class/system.h"
#include "vi/vedam.h"
#include "vti/class_uuid.h"

#include <stdio.h>
#include <string.h>

static int g_fails;

static void check(int cond, const char *file, int line, const char *expr)
{
    if (!cond) {
        fprintf(stderr, "FAIL %s:%d %s\n", file, line, expr);
        g_fails++;
    }
}

#define CHECK(cond) check((cond), __FILE__, __LINE__, #cond)

static v_status_t get_one(v_status_t (*fn)(vi_msg_t *), v_attr_id_t id, const void *key,
                          vuint32_t key_len, v_status_t *out_st)
{
    v_attr_t attr;
    v_status_t status;
    vi_msg_t vim;
    v_status_t rc;

    memset(&attr, 0, sizeof(attr));
    memset(&status, 0, sizeof(status));
    memset(&vim, 0, sizeof(vim));
    attr.id = id;
    vim.key = key;
    vim.key_len = key_len;
    vim.attr_count = 1;
    vim.attr_list = &attr;
    vim.status_list = &status;
    rc = fn(&vim);
    if (out_st != NULL) {
        *out_st = status;
    }
    return rc;
}

/**
 * @brief Call @p fn with an empty attribute list.
 *
 * @param[in] fn Public verb
 * @return The call status.
 */
static v_status_t get_empty(v_status_t (*fn)(vi_msg_t *))
{
    vi_msg_t vim;

    memset(&vim, 0, sizeof(vim));
    return fn(&vim);
}

/* RFC 9562 UUID v5 vectors from the Round 6 review packet. */
static const uint8_t k_system_uuid[16] = {
    0xc2, 0xb9, 0x15, 0x80, 0x60, 0x92, 0x5c, 0x64, 0xbd, 0x0c, 0xea, 0x7f, 0x37, 0x7b, 0x56, 0xc1,
};
static const uint8_t k_identity_uuid[16] = {
    0x29, 0xeb, 0x31, 0x2d, 0x05, 0x7d, 0x59, 0x30, 0xbc, 0x70, 0xbc, 0x6b, 0x01, 0xcb, 0x45, 0xab,
};
static const uint8_t k_storage_uuid[16] = {
    0x77, 0x14, 0x5e, 0x11, 0xa4, 0x7c, 0x50, 0xda, 0xbc, 0x42, 0xa9, 0x1a, 0xd8, 0xd7, 0x64, 0xb8,
};
static const uint8_t k_security_uuid[16] = {
    0x3d, 0x8d, 0x56, 0x23, 0x8c, 0xa8, 0x58, 0x5f, 0xa4, 0x34, 0x66, 0x4c, 0x1d, 0xf3, 0x8f, 0xfa,
};
static const uint8_t k_contract_uuid[16] = {
    0xf8, 0xc5, 0xc5, 0x5f, 0x54, 0xb9, 0x5e, 0x5b, 0xad, 0xea, 0xd3, 0x92, 0xc8, 0xb4, 0x8c, 0xcd,
};
static const uint8_t k_plugin_uuid[16] = {
    0xc8, 0xb1, 0xba, 0x03, 0xa6, 0x73, 0x5b, 0xa4, 0x95, 0x04, 0x45, 0x23, 0xcf, 0xaf, 0xc7, 0x62,
};

static int test_uuids(void)
{
    v_class_id_t id;
    vuint32_t major;
    vuint32_t minor;
    v_status_t st;

    CHECK(memcmp(vi_system_uuid, k_system_uuid, 16) == 0);
    CHECK(memcmp(vi_identity_uuid, k_identity_uuid, 16) == 0);
    CHECK(memcmp(vi_storage_uuid, k_storage_uuid, 16) == 0);
    CHECK(memcmp(vi_security_uuid, k_security_uuid, 16) == 0);
    CHECK(memcmp(vi_contract_uuid, k_contract_uuid, 16) == 0);
    CHECK(memcmp(vi_plugin_uuid, k_plugin_uuid, 16) == 0);

    st = vedam_init();
    CHECK(st == V_STATUS_SUCCESS);
    if (st != V_STATUS_SUCCESS) {
        return g_fails;
    }

    st = vti_class_from_uuid(vi_system_uuid, &id, &major, &minor);
    CHECK(st == V_STATUS_SUCCESS && id == V_CLASS_SYSTEM && major == 1 && minor == 0);
    st = vti_class_from_uuid(vi_identity_uuid, &id, &major, &minor);
    CHECK(st == V_STATUS_SUCCESS && id == V_CLASS_IDENTITY && major == 1 && minor == 0);
    st = vti_class_from_uuid(vi_storage_uuid, &id, &major, &minor);
    CHECK(st == V_STATUS_SUCCESS && id == V_CLASS_STORAGE && major == 1 && minor == 0);
    st = vti_class_from_uuid(vi_security_uuid, &id, &major, &minor);
    CHECK(st == V_STATUS_SUCCESS && id == V_CLASS_SECURITY && major == 1 && minor == 0);
    st = vti_class_from_uuid(vi_contract_uuid, &id, &major, &minor);
    CHECK(st == V_STATUS_SUCCESS && id == V_CLASS_CONTRACT && major == 1 && minor == 0);
    st = vti_class_from_uuid(vi_plugin_uuid, &id, &major, &minor);
    CHECK(st == V_STATUS_SUCCESS && id == V_CLASS_PLUGIN && major == 1 && minor == 0);

    vedam_deinit();
    return g_fails;
}

static int test_gets(void)
{
    v_status_t rc;
    v_status_t st;
    const char *k1 = "id-a";
    const char *k2 = "id-b";
    const char *s1 = "st-a";
    const char *s2 = "st-b";

    rc = vedam_init();
    CHECK(rc == V_STATUS_SUCCESS);
    if (rc != V_STATUS_SUCCESS) {
        return g_fails;
    }

    rc = get_one(vi_system_get, VI_SYSTEM_ATTR_NODE_CLASS, NULL, 0, &st);
    CHECK(rc == V_STATUS_SUCCESS && st == V_STATUS_SUCCESS);

    rc = get_one(vi_identity_get, VI_IDENTITY_ATTR_KIND, k1, (vuint32_t)strlen(k1), &st);
    CHECK(rc == V_STATUS_SUCCESS && st == V_STATUS_SUCCESS);
    rc = get_one(vi_identity_get, VI_IDENTITY_ATTR_KIND, k2, (vuint32_t)strlen(k2), &st);
    CHECK(rc == V_STATUS_SUCCESS && st == V_STATUS_SUCCESS);

    rc = get_one(vi_storage_get, VI_STORAGE_ATTR_SIZE, s1, (vuint32_t)strlen(s1), &st);
    CHECK(rc == V_STATUS_SUCCESS && st == V_STATUS_SUCCESS);
    rc = get_one(vi_storage_get, VI_STORAGE_ATTR_SIZE, s2, (vuint32_t)strlen(s2), &st);
    CHECK(rc == V_STATUS_SUCCESS && st == V_STATUS_SUCCESS);

    rc = get_one(vi_security_get, VI_SECURITY_ATTR_PROFILE, NULL, 0, &st);
    CHECK(rc == V_STATUS_SUCCESS && st == V_STATUS_SUCCESS);

    rc = get_empty(vi_contract_get);
#if VEDAM_ENABLE_CONTRACT
    CHECK(rc == V_STATUS_NOT_IMPLEMENTED);
#else
    CHECK(rc == V_STATUS_NOT_SUPPORTED);
#endif
    rc = get_empty(vi_plugin_get);
#if VEDAM_ENABLE_PLUGIN
    CHECK(rc == V_STATUS_NOT_IMPLEMENTED);
#else
    CHECK(rc == V_STATUS_NOT_SUPPORTED);
#endif

    vedam_deinit();
    return g_fails;
}

static int test_miss_hits(void)
{
    v_status_t rc;
    v_status_t st;
    unsigned long before;
    unsigned long after;
    const char *k1 = "hit-key";
    const char *k2 = "new-key";

    rc = vedam_init();
    CHECK(rc == V_STATUS_SUCCESS);
    if (rc != V_STATUS_SUCCESS) {
        return g_fails;
    }

    rc = get_one(vi_system_get, VI_SYSTEM_ATTR_NODE_CLASS, NULL, 0, &st);
    CHECK(rc == V_STATUS_SUCCESS && st == V_STATUS_SUCCESS);

    before = vedam_lock_object_misses();
    rc = get_one(vi_system_get, VI_SYSTEM_ATTR_NODE_CLASS, NULL, 0, &st);
    CHECK(rc == V_STATUS_SUCCESS && st == V_STATUS_SUCCESS);
    after = vedam_lock_object_misses();
    CHECK(after == before);

    rc = get_one(vi_identity_get, VI_IDENTITY_ATTR_KIND, k1, (vuint32_t)strlen(k1), &st);
    CHECK(rc == V_STATUS_SUCCESS && st == V_STATUS_SUCCESS);

    before = vedam_lock_object_misses();
    rc = get_one(vi_identity_get, VI_IDENTITY_ATTR_KIND, k1, (vuint32_t)strlen(k1), &st);
    CHECK(rc == V_STATUS_SUCCESS && st == V_STATUS_SUCCESS);
    after = vedam_lock_object_misses();
    CHECK(after == before);

    before = vedam_lock_object_misses();
    rc = get_one(vi_identity_get, VI_IDENTITY_ATTR_KIND, k2, (vuint32_t)strlen(k2), &st);
    CHECK(rc == V_STATUS_SUCCESS && st == V_STATUS_SUCCESS);
    after = vedam_lock_object_misses();
    CHECK(after == before + 1);

    vedam_deinit();
    return g_fails;
}

int vedam_legacy_verticals(void)
{
    if (test_uuids() != 0) {
        return 1;
    }
    g_fails = 0;
    if (test_gets() != 0) {
        return 1;
    }
    g_fails = 0;
#if VEDAM_ENABLE_WWLOCK
    if (test_miss_hits() != 0) {
        return 1;
    }
#endif
    printf("verticals tests passed\n");
    return 0;
}

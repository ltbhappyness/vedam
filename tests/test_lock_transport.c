/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * Two classes lock through the local transport in opposite order, and two
 * system attributes are read together.
 */
#include "core/core.h"
#include "lock/store.h"
#include "vii/class/identity.h"
#include "vii/class/storage.h"
#include "vti/dispatch.h"
#include "vi/class/identity.h"
#include "vi/class/storage.h"
#include "vi/class/system.h"
#include "vi/vedam.h"

#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int g_fails;

static void check(int cond, const char *file, int line, const char *expr)
{
    if (!cond) {
        fprintf(stderr, "FAIL %s:%d %s\n", file, line, expr);
        g_fails++;
    }
}

#define CHECK(cond) check((cond), __FILE__, __LINE__, #cond)

struct cross {
    v_class_id_t other;
    const char *other_key;
    vuint32_t other_key_len;
    const char *mine_key;
    vuint32_t mine_key_len;
    atomic_int *arrived;
    v_status_t status;
};

static v_status_t cross_body(void *user)
{
    struct cross *cross = (struct cross *)user;
    vuint32_t major;
    vuint32_t minor;
    v_attr_t attr;
    v_status_t st;

    memset(&attr, 0, sizeof(attr));
    attr.id = 1;
    if (cross->other == V_CLASS_IDENTITY) {
        major = VII_IDENTITY_MAJOR;
        minor = VII_IDENTITY_MINOR;
    } else {
        major = VII_STORAGE_MAJOR;
        minor = VII_STORAGE_MINOR;
    }
    atomic_fetch_add_explicit(cross->arrived, 1, memory_order_release);
    while (atomic_load_explicit(cross->arrived, memory_order_acquire) < 2) {
        sched_yield();
    }
    {
        vi_msg_t vim;
        vta_msg_t call;
        v_status_t status;

        memset(&status, 0, sizeof(status));
        memset(&vim, 0, sizeof(vim));
        memset(&call, 0, sizeof(call));
        vim.key = cross->other_key;
        vim.key_len = cross->other_key_len;
        vim.attr_count = 1;
        vim.attr_list = &attr;
        vim.status_list = &status;
        call.class_id = cross->other;
        call.origin = V_ORIGIN_INTERNAL;
        call.verb = V_VERB_SET;
        call.major = major;
        call.minor = minor;
        call.msg = &vim;
        st = vta_dispatch(&call);
    }
    return st;
}

static void *cross_thread(void *raw)
{
    struct cross *cross = (struct cross *)raw;
    v_attr_t attr;
    v_status_t status;
    v_class_id_t mine;

    memset(&attr, 0, sizeof(attr));
    memset(&status, 0, sizeof(status));
    attr.id = 1;
    mine = cross->other == V_CLASS_STORAGE ? V_CLASS_IDENTITY : V_CLASS_STORAGE;
    cross->status = vedam_lock_invoke(mine, cross->mine_key, cross->mine_key_len, V_VERB_SET,
                                      V_ORIGIN_INTERNAL, 1, &attr, &status, cross_body, cross);
    return NULL;
}

static int test_two_classes(void)
{
    struct cross a;
    struct cross b;
    pthread_t threads[2];
    atomic_int arrived;
    v_status_t rc;
    const char *ka = "lock-a";
    const char *kb = "lock-b";

    fprintf(stderr, "test two classes\n");
    atomic_store(&arrived, 0);
    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        fprintf(stderr, "init %d\n", (int)rc);
        return 1;
    }
    a.other = V_CLASS_STORAGE;
    a.other_key = kb;
    a.other_key_len = (vuint32_t)strlen(kb);
    a.mine_key = ka;
    a.mine_key_len = (vuint32_t)strlen(ka);
    b.other = V_CLASS_IDENTITY;
    b.other_key = ka;
    b.other_key_len = (vuint32_t)strlen(ka);
    b.mine_key = kb;
    b.mine_key_len = (vuint32_t)strlen(kb);
    a.arrived = b.arrived = &arrived;
    a.status = b.status = V_STATUS_FAILURE;
    pthread_create(&threads[0], NULL, cross_thread, &a);
    pthread_create(&threads[1], NULL, cross_thread, &b);
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    CHECK(a.status == V_STATUS_SUCCESS);
    CHECK(b.status == V_STATUS_SUCCESS);
    vedam_deinit();
    return g_fails;
}

struct read_arg {
    v_attr_id_t id;
    int loops;
    v_status_t status;
};

static void *read_thread(void *raw)
{
    struct read_arg *arg = (struct read_arg *)raw;
    v_attr_t attr;
    int i;

    arg->status = V_STATUS_SUCCESS;
    for (i = 0; i < arg->loops; i++) {
        v_status_t rc;

        {
            v_status_t status;
            vi_msg_t vim;

            memset(&attr, 0, sizeof(attr));
            memset(&status, 0, sizeof(status));
            memset(&vim, 0, sizeof(vim));
            attr.id = arg->id;
            vim.attr_count = 1;
            vim.attr_list = &attr;
            vim.status_list = &status;
            rc = vi_system_get(&vim);
            if (rc != V_STATUS_SUCCESS || status != V_STATUS_SUCCESS) {
                arg->status = rc != V_STATUS_SUCCESS ? rc : status;
                break;
            }
        }
    }
    return NULL;
}

static int test_two_attrs(void)
{
    struct read_arg a;
    struct read_arg b;
    pthread_t threads[2];
    v_status_t rc;

    fprintf(stderr, "test two attributes\n");
    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        fprintf(stderr, "init %d\n", (int)rc);
        return 1;
    }
    a.id = VI_SYSTEM_ATTR_NODE_CLASS;
    b.id = VI_SYSTEM_ATTR_PROTOCOL_VERSION;
    a.loops = b.loops = 40;
    a.status = b.status = V_STATUS_FAILURE;
    pthread_create(&threads[0], NULL, read_thread, &a);
    pthread_create(&threads[1], NULL, read_thread, &b);
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    CHECK(a.status == V_STATUS_SUCCESS);
    CHECK(b.status == V_STATUS_SUCCESS);
    vedam_deinit();
    return g_fails;
}

int main(void)
{
    alarm(30);
    if (test_two_classes() != 0) {
        return 1;
    }
    g_fails = 0;
    if (test_two_attrs() != 0) {
        return 1;
    }
    printf("lock transport tests passed\n");
    return 0;
}

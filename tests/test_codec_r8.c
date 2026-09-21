/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * Round 8: local get skips the codec; queue and HTTP carry per-attribute status;
 * two HTTP calls on one connected descriptor share one TCP socket.
 */
#include "core/core.h"
#include "vi/class/system.h"
#include "vi/vedam.h"
#include "vii/class/system.h"
#include "vti/codec.h"
#include "vti/grpc.h"
#include "vti/mq.h"
#include "vti/vta.h"

#include <stdio.h>
#include <string.h>

/**
 * @brief Print a failure and return 1.
 *
 * @param[in] msg Failure label
 * @param[in] rc  Status code
 * @return 1.
 */
static int fail(const char *msg, v_status_t rc)
{
    fprintf(stderr, "%s: %d\n", msg, (int)rc);
    return 1;
}

/**
 * @brief Show that a local vi_system_get fills status_list without a transport.
 *
 * @return 0 on success, 1 on failure.
 */
static int test_local_skips_codec(void)
{
    v_attr_t attr;
    v_status_t status;
    vi_msg_t vim;
    v_status_t rc;

    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        return fail("init local", rc);
    }

    memset(&attr, 0, sizeof(attr));
    memset(&status, 0, sizeof(status));
    memset(&vim, 0, sizeof(vim));
    attr.id = VI_SYSTEM_ATTR_NODE_CLASS;
    status = (v_status_t)0x7fff;
    vim.attr_count = 1;
    vim.attr_list = &attr;
    vim.status_list = &status;
    rc = vi_system_get(&vim);
    if (rc != V_STATUS_SUCCESS) {
        vedam_deinit();
        return fail("local get", rc);
    }
    if (status != V_STATUS_SUCCESS) {
        vedam_deinit();
        return fail("local status_list", status);
    }

    vedam_deinit();
    return 0;
}

/**
 * @brief Build a two-attribute get with one unknown id.
 *
 * @param[out] attrs    Attribute array of length 2
 * @param[out] statuses Status array of length 2
 * @param[out] vim      Public message
 * @param[out] msg      Transport message
 */
static void fill_mixed(v_attr_t *attrs, v_status_t *statuses, vi_msg_t *vim, vta_msg_t *msg)
{
    memset(attrs, 0, sizeof(attrs[0]) * 2);
    memset(statuses, 0, sizeof(statuses[0]) * 2);
    memset(vim, 0, sizeof(*vim));
    memset(msg, 0, sizeof(*msg));
    attrs[0].id = VI_SYSTEM_ATTR_NODE_CLASS;
    attrs[1].id = 9999;
    vim->attr_count = 2;
    vim->attr_list = attrs;
    vim->status_list = statuses;
    msg->class_id = V_CLASS_SYSTEM;
    msg->origin = V_ORIGIN_EXTERNAL;
    msg->verb = V_VERB_GET;
    msg->major = VII_SYSTEM_MAJOR;
    msg->minor = VII_SYSTEM_MINOR;
    msg->msg = vim;
}

/**
 * @brief Check mixed success and unknown-attribute statuses.
 *
 * @param[in] rc       Overall status
 * @param[in] statuses Per-attribute statuses
 * @return 0 when the codes match, 1 otherwise.
 */
static int expect_mixed(v_status_t rc, const v_status_t *statuses)
{
    v_status_t expect;

    expect = (v_status_t)(V_STATUS_UNKNOWN_ATTRIBUTE_0 - 1);
    if (rc != expect) {
        return fail("overall", rc);
    }
    if (statuses[0] != V_STATUS_SUCCESS) {
        return fail("status0", statuses[0]);
    }
    if (statuses[1] != expect) {
        return fail("status1", statuses[1]);
    }
    return 0;
}

/**
 * @brief Carry mixed statuses over the message queue.
 *
 * @return 0 on success, 1 on failure.
 */
static int test_mq_mixed(void)
{
    v_attr_t attrs[2];
    v_status_t statuses[2];
    vi_msg_t vim;
    vta_msg_t msg;
    vta_desc_t *tx;
    const char *qname;
    v_status_t rc;

    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        return fail("init mq", rc);
    }

    tx = vta_open(VTA_LINK_IPC);
    if (tx == NULL) {
        vedam_deinit();
        return fail("mq open", V_STATUS_NO_MEMORY);
    }
    qname = vta_mq_bound(vedam_node()->rx[VTA_LINK_IPC]);
    if (qname == NULL) {
        vta_close(tx);
        vedam_deinit();
        return fail("mq bound", V_STATUS_FAILURE);
    }
    rc = vta_connect(tx, qname);
    if (rc != V_STATUS_SUCCESS) {
        vta_close(tx);
        vedam_deinit();
        return fail("mq connect", rc);
    }

    fill_mixed(attrs, statuses, &vim, &msg);
    rc = vta_call(tx, &msg);
    if (expect_mixed(rc, statuses) != 0) {
        vta_close(tx);
        vedam_deinit();
        return 1;
    }

    vta_close(tx);
    vedam_deinit();
    return 0;
}

/**
 * @brief Carry mixed statuses over the temporary HTTP client path.
 *
 * @return 0 on success, 1 on failure.
 */
static int test_http_mixed(void)
{
    v_attr_t attrs[2];
    v_status_t statuses[2];
    vi_msg_t vim;
    vta_msg_t msg;
    vta_desc_t *tx;
    const char *bound;
    v_status_t rc;

    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        return fail("init http", rc);
    }

    bound = vta_grpc_bound(vedam_node()->rx[VTA_LINK_REMOTE]);
    if (bound == NULL) {
        vedam_deinit();
        return fail("http bound", V_STATUS_FAILURE);
    }

    tx = vta_open(VTA_LINK_REMOTE);
    if (tx == NULL) {
        vedam_deinit();
        return fail("http open", V_STATUS_NO_MEMORY);
    }
    rc = vta_connect(tx, bound);
    if (rc != V_STATUS_SUCCESS) {
        vta_close(tx);
        vedam_deinit();
        return fail("http connect", rc);
    }

    fill_mixed(attrs, statuses, &vim, &msg);
    rc = vta_call(tx, &msg);
    if (expect_mixed(rc, statuses) != 0) {
        vta_close(tx);
        vedam_deinit();
        return 1;
    }

    vta_close(tx);
    vedam_deinit();
    return 0;
}

/**
 * @brief Two calls on one connected descriptor must not open a second TCP socket.
 *
 * @return 0 on success, 1 on failure.
 */
static int test_http_one_socket(void)
{
    v_attr_t attrs[2];
    v_status_t statuses[2];
    vi_msg_t vim;
    vta_msg_t msg;
    vta_desc_t *tx;
    const char *bound;
    unsigned connects;
    v_status_t rc;

    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        return fail("init one socket", rc);
    }

    bound = vta_grpc_bound(vedam_node()->rx[VTA_LINK_REMOTE]);
    if (bound == NULL) {
        vedam_deinit();
        return fail("one socket bound", V_STATUS_FAILURE);
    }

    tx = vta_open(VTA_LINK_REMOTE);
    if (tx == NULL) {
        vedam_deinit();
        return fail("one socket open", V_STATUS_NO_MEMORY);
    }
    rc = vta_connect(tx, bound);
    if (rc != V_STATUS_SUCCESS) {
        vta_close(tx);
        vedam_deinit();
        return fail("one socket connect", rc);
    }

    connects = vta_grpc_tcp_connects(tx);
    if (connects != 1) {
        vta_close(tx);
        vedam_deinit();
        return fail("connect count after connect", (v_status_t)connects);
    }

    fill_mixed(attrs, statuses, &vim, &msg);
    rc = vta_call(tx, &msg);
    if (expect_mixed(rc, statuses) != 0) {
        vta_close(tx);
        vedam_deinit();
        return 1;
    }

    fill_mixed(attrs, statuses, &vim, &msg);
    rc = vta_call(tx, &msg);
    if (expect_mixed(rc, statuses) != 0) {
        vta_close(tx);
        vedam_deinit();
        return 1;
    }

    connects = vta_grpc_tcp_connects(tx);
    if (connects != 1) {
        vta_close(tx);
        vedam_deinit();
        return fail("second TCP connect", (v_status_t)connects);
    }

    vta_close(tx);
    vedam_deinit();
    return 0;
}

/**
 * @brief Run Round 8 codec path checks.
 *
 * @return 0 on success, 1 on failure.
 */
int main(void)
{
    if (test_local_skips_codec() != 0) {
        return 1;
    }
    if (test_mq_mixed() != 0) {
        return 1;
    }
    if (test_http_mixed() != 0) {
        return 1;
    }
    if (test_http_one_socket() != 0) {
        return 1;
    }
    return 0;
}

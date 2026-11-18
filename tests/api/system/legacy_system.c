/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * System class: init lifecycle, public get/subscribe, VIA rejects, origin gates.
 */
#include "core/node.h"
#include "vi/class/system.h"
#include "vi/vedam.h"
#include "vii/class/system.h"
#include "vii/meta.h"
#include "vti/mq.h"
#include "vti/vti.h"

#include <stdio.h>
#include <string.h>

static int fail(const char *msg, v_status_t rc)
{
    fprintf(stderr, "%s: %d\n", msg, (int)rc);
    return 1;
}

/**
 * @brief Run @p verb on the System row the way a caller with @p origin would.
 *
 * @param[in]     origin Call origin
 * @param[in]     verb   Verb
 * @param[in,out] vim    Call message
 * @return The verb status.
 */
static v_status_t system_verb(v_origin_t origin, v_verb_t verb, vi_msg_t *vim)
{
    vii_msg_t call;

    memset(&call, 0, sizeof(call));
    call.class_id = V_CLASS_SYSTEM;
    call.origin = origin;
    call.verb = verb;
    call.major = VII_SYSTEM_MAJOR;
    call.minor = VII_SYSTEM_MINOR;
    call.msg = vim;
    return vii_verb(&vii_classes[V_CLASS_SYSTEM], &call);
}

static int test_origin_gates(void)
{
    vi_msg_t vim;
    v_status_t rc;

    memset(&vim, 0, sizeof(vim));
    rc = system_verb(V_ORIGIN_EXTERNAL, V_VERB_CREATE, &vim);
    if (rc != V_STATUS_NOT_IMPLEMENTED) {
        return fail("external create", rc);
    }

    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        return fail("init for origin", rc);
    }

    rc = system_verb(V_ORIGIN_EXTERNAL, V_VERB_SET, &vim);
    if (rc != V_STATUS_NOT_IMPLEMENTED) {
        vedam_deinit();
        return fail("external set", rc);
    }

    rc = system_verb(V_ORIGIN_EXTERNAL, V_VERB_DELETE, &vim);
    if (rc != V_STATUS_NOT_IMPLEMENTED) {
        vedam_deinit();
        return fail("external delete", rc);
    }

    vedam_deinit();
    return 0;
}

static int check_statuses(const v_status_t *statuses, vuint32_t n, v_status_t expect)
{
    vuint32_t i;

    for (i = 0; i < n; i++) {
        if (statuses[i] != expect) {
            return 0;
        }
    }
    return 1;
}

#if VEDAM_ENABLE_MQ
/**
 * @brief Get System over the message queue while the node is up.
 *
 * @return 0 on success, 1 on failure.
 */
static int test_mq_get(void)
{
    vti_desc_t *tx;
    vii_msg_t msg;
    vi_msg_t one_vim;
    v_attr_t one;
    v_status_t one_st;
    const char *qname;
    v_status_t rc;

    tx = vti_open(VTI_LINK_IPC);
    if (tx == NULL) {
        vedam_deinit();
        return fail("mq open", V_STATUS_NO_MEMORY);
    }
    qname = vti_mq_bound(vedam_node()->rx[VTI_LINK_IPC]);
    if (qname == NULL) {
        vti_close(tx);
        vedam_deinit();
        return fail("mq bound", V_STATUS_FAILURE);
    }
    rc = vti_connect(tx, qname);
    if (rc != V_STATUS_SUCCESS) {
        vti_close(tx);
        vedam_deinit();
        return fail("mq connect", rc);
    }
    memset(&one, 0, sizeof(one));
    memset(&one_st, 0, sizeof(one_st));
    memset(&one_vim, 0, sizeof(one_vim));
    memset(&msg, 0, sizeof(msg));
    one.id = VI_SYSTEM_ATTR_NODE_CLASS;
    one_vim.attr_count = 1;
    one_vim.attr_list = &one;
    one_vim.status_list = &one_st;
    msg.class_id = V_CLASS_SYSTEM;
    msg.origin = V_ORIGIN_EXTERNAL;
    msg.verb = V_VERB_GET;
    msg.major = VII_SYSTEM_MAJOR;
    msg.minor = VII_SYSTEM_MINOR;
    msg.msg = &one_vim;
    rc = vti_call(tx, &msg);
    if (rc != V_STATUS_SUCCESS) {
        vti_close(tx);
        vedam_deinit();
        return fail("mq get", rc);
    }
    if (one_st != V_STATUS_SUCCESS) {
        vti_close(tx);
        vedam_deinit();
        return fail("mq get status_list", one_st);
    }
    vti_close(tx);
    return 0;
}
#endif

static int test_public_get(void)
{
    v_attr_t attrs[3];
    v_status_t statuses[3];
    vi_system_state_t state;
    vi_msg_t vim;
    v_status_t rc;

    memset(attrs, 0, sizeof(attrs));
    memset(statuses, 0, sizeof(statuses));
    memset(&vim, 0, sizeof(vim));
    attrs[0].id = VI_SYSTEM_ATTR_NODE_CLASS;
    vim.attr_count = 1;
    vim.attr_list = attrs;
    vim.status_list = statuses;
    rc = vi_system_get(&vim);
    if (rc != V_STATUS_UNINITIALIZED) {
        return fail("get before init", rc);
    }

    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        return fail("init", rc);
    }

    rc = vedam_init();
    if (rc != V_STATUS_ITEM_ALREADY_EXISTS) {
        vedam_deinit();
        return fail("second init", rc);
    }

    memset(attrs, 0, sizeof(attrs));
    memset(statuses, 0, sizeof(statuses));
    memset(&state, 0, sizeof(state));
    memset(&vim, 0, sizeof(vim));
    attrs[0].id = VI_SYSTEM_ATTR_NODE_CLASS;
    attrs[1].id = VI_SYSTEM_ATTR_PROTOCOL_VERSION;
    attrs[2].id = VI_SYSTEM_ATTR_SYSTEM_STATE;
    attrs[2].value.ptr = &state;
    vim.attr_count = 3;
    vim.attr_list = attrs;
    vim.status_list = statuses;

    rc = vi_system_get(&vim);
    if (rc != V_STATUS_SUCCESS) {
        vedam_deinit();
        return fail("get", rc);
    }
    if (!check_statuses(statuses, 3, V_STATUS_SUCCESS)) {
        vedam_deinit();
        return fail("get status_list", rc);
    }

#if VEDAM_ENABLE_MQ
    if (test_mq_get() != 0) {
        return 1;
    }
#endif

    memset(attrs, 0, sizeof(attrs[0]));
    memset(statuses, 0, sizeof(statuses[0]));
    memset(&vim, 0, sizeof(vim));
    attrs[0].id = VI_SYSTEM_ATTR_PROTOCOL_VERSION;
    vim.attr_count = 1;
    vim.attr_list = attrs;
    vim.status_list = statuses;
    rc = vi_system_subscribe(&vim);
    if (rc != V_STATUS_SUCCESS) {
        vedam_deinit();
        return fail("subscribe version", rc);
    }
    if (statuses[0] != V_STATUS_SUCCESS) {
        vedam_deinit();
        return fail("subscribe version status", statuses[0]);
    }

    attrs[0].id = VI_SYSTEM_ATTR_NODE_CLASS;
    statuses[0] = V_STATUS_SUCCESS;
    rc = vi_system_subscribe(&vim);
    if (rc >= 0) {
        vedam_deinit();
        return fail("subscribe node class", rc);
    }
    if (statuses[0] == V_STATUS_SUCCESS) {
        vedam_deinit();
        return fail("subscribe node class status", statuses[0]);
    }

    rc = vedam_deinit();
    if (rc != V_STATUS_SUCCESS) {
        return fail("deinit", rc);
    }
    return 0;
}

static int test_via_rejects(void)
{
    v_attr_t attr;
    v_status_t status;
    vi_msg_t vim;
    v_status_t rc;

    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        return fail("init for via", rc);
    }

    memset(&attr, 0, sizeof(attr));
    memset(&status, 0, sizeof(status));
    memset(&vim, 0, sizeof(vim));
    attr.id = 9999;
    vim.attr_count = 1;
    vim.attr_list = &attr;
    vim.status_list = &status;
    rc = vi_system_get(&vim);
    if (rc != (v_status_t)(V_STATUS_UNKNOWN_ATTRIBUTE_0)) {
        vedam_deinit();
        return fail("unknown id", rc);
    }
    if (status != (v_status_t)(V_STATUS_UNKNOWN_ATTRIBUTE_0)) {
        vedam_deinit();
        return fail("unknown id status", status);
    }

    memset(&attr, 0, sizeof(attr));
    memset(&status, 0, sizeof(status));
    attr.id = 5;
    vim.attr_count = 1;
    vim.attr_list = &attr;
    vim.status_list = &status;
    rc = vi_system_get(&vim);
    if (rc != (v_status_t)(V_STATUS_UNKNOWN_ATTRIBUTE_0)) {
        vedam_deinit();
        return fail("unknown id 5", rc);
    }

    memset(&attr, 0, sizeof(attr));
    memset(&status, 0, sizeof(status));
    attr.id = VI_SYSTEM_ATTR_NODE_CLASS;
    vim.attr_count = 1;
    vim.attr_list = &attr;
    vim.status_list = &status;
    rc = vi_system_subscribe(&vim);
    if (rc != (v_status_t)(V_STATUS_ATTR_NOT_SUPPORTED_0)) {
        vedam_deinit();
        return fail("subscribe read-only non-sub", rc);
    }
    if (status != (v_status_t)(V_STATUS_ATTR_NOT_SUPPORTED_0)) {
        vedam_deinit();
        return fail("subscribe read-only status", status);
    }

    rc = vedam_deinit();
    if (rc != V_STATUS_SUCCESS) {
        return fail("deinit via", rc);
    }
    return 0;
}

int vedam_legacy_system(void)
{
    if (test_origin_gates() != 0) {
        return 1;
    }
    if (test_public_get() != 0) {
        return 1;
    }
    if (test_via_rejects() != 0) {
        return 1;
    }
    return 0;
}

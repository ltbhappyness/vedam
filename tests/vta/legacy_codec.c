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
#include "core/node.h"
#include "util/attr.h"
#include "vi/class/system.h"
#include "vi/vedam.h"
#include "vii/class/system.h"
#include "vii/meta.h"
#include "vti/codec.h"
#include "vti/grpc.h"
#include "vti/mq.h"
#include "vti/vti.h"

#include <stdio.h>
#include <string.h>

enum {
    TEST_COMMIT_LEN = 40,
    TEST_TEXT_ROOM = 64,
    TEST_LIST_ID = 9,
    TEST_LIST_MAX = 8,
    TEST_LIST_LEN = 3,
    TEST_ROWS_MAX = 16
};

static const char test_commit[] = "0123456789abcdef0123456789abcdef01234567";
static const vuint32_t test_items[TEST_LIST_LEN] = {7, 8, 9};
static char full_text[VTI_CODEC_VALUE_MAX];

/* A copy of the class table whose System get body fills values. */
static struct vii_class test_classes[V_CLASS_ID_MAX + 1];

/* System rows plus one test-only U32 list row, since no product class has a list. */
static struct vii_attr_meta test_rows[TEST_ROWS_MAX];
static vuint8_t test_by_id[TEST_LIST_ID + 1];

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
static void fill_mixed(v_attr_t *attrs, v_status_t *statuses, vi_msg_t *vim, vii_msg_t *msg)
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

#if VEDAM_ENABLE_MQ
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
    vii_msg_t msg;
    vti_desc_t *tx;
    const char *qname;
    v_status_t rc;

    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        return fail("init mq", rc);
    }

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

    fill_mixed(attrs, statuses, &vim, &msg);
    rc = vti_call(tx, &msg);
    if (expect_mixed(rc, statuses) != 0) {
        vti_close(tx);
        vedam_deinit();
        return 1;
    }

    vti_close(tx);
    vedam_deinit();
    return 0;
}
#endif /* VEDAM_ENABLE_MQ */

#if VEDAM_ENABLE_GRPC
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
    vii_msg_t msg;
    vti_desc_t *tx;
    const char *bound;
    v_status_t rc;

    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        return fail("init http", rc);
    }

    bound = vti_grpc_bound(vedam_node()->rx[VTI_LINK_REMOTE]);
    if (bound == NULL) {
        vedam_deinit();
        return fail("http bound", V_STATUS_FAILURE);
    }

    tx = vti_open(VTI_LINK_REMOTE);
    if (tx == NULL) {
        vedam_deinit();
        return fail("http open", V_STATUS_NO_MEMORY);
    }
    rc = vti_connect(tx, bound);
    if (rc != V_STATUS_SUCCESS) {
        vti_close(tx);
        vedam_deinit();
        return fail("http connect", rc);
    }

    fill_mixed(attrs, statuses, &vim, &msg);
    rc = vti_call(tx, &msg);
    if (expect_mixed(rc, statuses) != 0) {
        vti_close(tx);
        vedam_deinit();
        return 1;
    }

    vti_close(tx);
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
    vii_msg_t msg;
    vti_desc_t *tx;
    const char *bound;
    unsigned connects;
    v_status_t rc;

    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        return fail("init one socket", rc);
    }

    bound = vti_grpc_bound(vedam_node()->rx[VTI_LINK_REMOTE]);
    if (bound == NULL) {
        vedam_deinit();
        return fail("one socket bound", V_STATUS_FAILURE);
    }

    tx = vti_open(VTI_LINK_REMOTE);
    if (tx == NULL) {
        vedam_deinit();
        return fail("one socket open", V_STATUS_NO_MEMORY);
    }
    rc = vti_connect(tx, bound);
    if (rc != V_STATUS_SUCCESS) {
        vti_close(tx);
        vedam_deinit();
        return fail("one socket connect", rc);
    }

    connects = vti_grpc_tcp_connects(tx);
    if (connects != 1) {
        vti_close(tx);
        vedam_deinit();
        return fail("connect count after connect", (v_status_t)connects);
    }

    fill_mixed(attrs, statuses, &vim, &msg);
    rc = vti_call(tx, &msg);
    if (expect_mixed(rc, statuses) != 0) {
        vti_close(tx);
        vedam_deinit();
        return 1;
    }

    fill_mixed(attrs, statuses, &vim, &msg);
    rc = vti_call(tx, &msg);
    if (expect_mixed(rc, statuses) != 0) {
        vti_close(tx);
        vedam_deinit();
        return 1;
    }

    connects = vti_grpc_tcp_connects(tx);
    if (connects != 1) {
        vti_close(tx);
        vedam_deinit();
        return fail("second TCP connect", (v_status_t)connects);
    }

    vti_close(tx);
    vedam_deinit();
    return 0;
}
#endif /* VEDAM_ENABLE_GRPC */

/**
 * @brief Test-only System get body: fills the node class, the state, and the commit text.
 *
 * The text goes through vattr_put_text, so the caller's room decides overflow.
 *
 * @param[in]     major Vertical major
 * @param[in]     minor Vertical minor
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS, or the first attribute failure.
 */
static v_status_t test_system_get(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    v_status_t first = V_STATUS_SUCCESS;
    v_status_t st;
    v_attr_t *attr;
    vuint32_t i;

    (void)major;
    (void)minor;
    for (i = 0; i < msg->attr_count; i++) {
        attr = &msg->attr_list[i];
        st = V_STATUS_SUCCESS;
        if (attr->id == VII_SYSTEM_ATTR_NODE_CLASS) {
            attr->value.s32 = VI_SYSTEM_NODE_CLASS_C;
        } else if (attr->id == VII_SYSTEM_ATTR_SYSTEM_STATE) {
            ((vi_system_state_t *)attr->value.ptr)->operational = VI_SYSTEM_STATE_READY;
        } else if (attr->id == VII_SYSTEM_ATTR_RELEASE_COMMIT) {
            st = vattr_put_text(attr, test_commit, TEST_COMMIT_LEN);
        } else if (attr->id == TEST_LIST_ID) {
            st = vattr_put_list(attr, test_items, TEST_LIST_LEN, sizeof(test_items[0]));
        }
        msg->status_list[i] = st;
        if (first == V_STATUS_SUCCESS) {
            first = st;
        }
    }
    return first;
}

/**
 * @brief Fill the full-slot text with non-zero bytes.
 */
static void prepare_full_text(void)
{
    int i;

    for (i = 0; i < VTI_CODEC_VALUE_MAX; i++) {
        full_text[i] = (char)(1 + (i % 251));
    }
}

/**
 * @brief Test-only System get body: writes one full slot of commit text.
 *
 * @param[in]     major Vertical major
 * @param[in]     minor Vertical minor
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS, or the first attribute failure.
 */
static v_status_t test_system_get_full(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    v_status_t first = V_STATUS_SUCCESS;
    v_status_t st;
    vuint32_t i;

    (void)major;
    (void)minor;
    for (i = 0; i < msg->attr_count; i++) {
        st = V_STATUS_SUCCESS;
        if (msg->attr_list[i].id == VII_SYSTEM_ATTR_RELEASE_COMMIT) {
            st = vattr_put_text(&msg->attr_list[i], full_text, VTI_CODEC_VALUE_MAX);
        }
        msg->status_list[i] = st;
        if (first == V_STATUS_SUCCESS) {
            first = st;
        }
    }
    return first;
}

/**
 * @brief Give the System row of @p cls the test list row and the test get body.
 *
 * @param[in,out] cls System row copied from the generated table
 * @return 0 on success, 1 when the System rows do not fit the test storage.
 */
static int add_test_rows(struct vii_class *cls)
{
    if (cls->attr_count + 1 > TEST_ROWS_MAX || cls->attr_id_max >= TEST_LIST_ID) {
        return 1;
    }
    memset(test_rows, 0, sizeof(test_rows));
    memset(test_by_id, 0, sizeof(test_by_id));
    memcpy(test_rows, cls->attrs, sizeof(test_rows[0]) * cls->attr_count);
    memcpy(test_by_id, cls->attr_by_id, (size_t)cls->attr_id_max + 1);
    test_rows[cls->attr_count].id = TEST_LIST_ID;
    test_rows[cls->attr_count].kind = VII_T_LIST;
    test_rows[cls->attr_count].is_public = 1;
    test_rows[cls->attr_count].elem_kind = VII_T_U32;
    test_rows[cls->attr_count].elem_size = sizeof(vuint32_t);
    test_rows[cls->attr_count].max_count = TEST_LIST_MAX;
    test_by_id[TEST_LIST_ID] = (vuint8_t)(cls->attr_count + 1);
    cls->attrs = test_rows;
    cls->attr_by_id = test_by_id;
    cls->attr_id_max = TEST_LIST_ID;
    cls->attr_count++;
    cls->body[V_VERB_GET] = test_system_get;
    return 0;
}

/**
 * @brief Start the node and point it at the class table copy with the test get body.
 *
 * @return V_STATUS_SUCCESS, or the vedam_init failure.
 */
static v_status_t start_with_test_get(void)
{
    v_status_t rc;

    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        return rc;
    }
    memcpy(test_classes, vii_classes, sizeof(test_classes));
    if (add_test_rows(&test_classes[V_CLASS_SYSTEM]) != 0) {
        vedam_deinit();
        return V_STATUS_FAILURE;
    }
    vedam_node()->classes = test_classes;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Close @p tx, restore the generated class table, and stop the node.
 *
 * @param[in] tx Client descriptor, or NULL
 */
static void stop_with_test_get(vti_desc_t *tx)
{
    if (tx != NULL) {
        vti_close(tx);
    }
    vedam_node()->classes = vii_classes;
    vedam_deinit();
}

/**
 * @brief Open and connect a client descriptor to this node's listener on @p link.
 *
 * @param[in] link VTI_LINK_IPC or VTI_LINK_REMOTE
 * @return Connected descriptor, or NULL.
 */
static vti_desc_t *connect_link(int link)
{
    const char *addr;
    vti_desc_t *tx;

    if (link == VTI_LINK_IPC) {
#if VEDAM_ENABLE_MQ
        addr = vti_mq_bound(vedam_node()->rx[VTI_LINK_IPC]);
#else
        addr = NULL;
#endif
    } else {
#if VEDAM_ENABLE_GRPC
        addr = vti_grpc_bound(vedam_node()->rx[VTI_LINK_REMOTE]);
#else
        addr = NULL;
#endif
    }
    if (addr == NULL) {
        return NULL;
    }
    tx = vti_open(link);
    if (tx == NULL) {
        return NULL;
    }
    if (vti_connect(tx, addr) != V_STATUS_SUCCESS) {
        vti_close(tx);
        return NULL;
    }
    return tx;
}

/**
 * @brief Build a System call for the state and the commit, with @p room bytes for the text.
 *
 * @param[out] attrs    Attribute array of length 2
 * @param[out] statuses Status array of length 2
 * @param[out] vim      Public message
 * @param[out] state    State the first attribute points at
 * @param[in]  buf      Text buffer for the second attribute
 * @param[in]  room     Bytes of room in @p buf
 */
static void fill_values(v_attr_t *attrs, v_status_t *statuses, vi_msg_t *vim,
                        vi_system_state_t *state, char *buf, vuint32_t room)
{
    memset(attrs, 0, sizeof(attrs[0]) * 2);
    memset(statuses, 0, sizeof(statuses[0]) * 2);
    memset(vim, 0, sizeof(*vim));
    memset(state, 0, sizeof(*state));
    attrs[0].id = VI_SYSTEM_ATTR_SYSTEM_STATE;
    attrs[0].value.ptr = state;
    attrs[1].id = VI_SYSTEM_ATTR_RELEASE_COMMIT;
    attrs[1].value.text.buf = buf;
    attrs[1].value.text.len = room;
    vim->attr_count = 2;
    vim->attr_list = attrs;
    vim->status_list = statuses;
}

/**
 * @brief Wrap @p vim as an external System call for @p verb.
 *
 * @param[out] msg  Transport message
 * @param[in]  vim  Public message
 * @param[in]  verb Verb
 */
static void system_msg(vii_msg_t *msg, vi_msg_t *vim, v_verb_t verb)
{
    memset(msg, 0, sizeof(*msg));
    msg->class_id = V_CLASS_SYSTEM;
    msg->origin = V_ORIGIN_EXTERNAL;
    msg->verb = verb;
    msg->major = VII_SYSTEM_MAJOR;
    msg->minor = VII_SYSTEM_MINOR;
    msg->msg = vim;
}

/**
 * @brief Get the state and the commit locally and over @p link, and compare them.
 *
 * @param[in] link VTI_LINK_IPC or VTI_LINK_REMOTE
 * @return 0 on success, 1 on failure.
 */
int vedam_vta_values(int link)
{
    char local_buf[TEST_TEXT_ROOM];
    char wire_buf[TEST_TEXT_ROOM];
    vi_system_state_t local_state;
    vi_system_state_t wire_state;
    v_attr_t attrs[2];
    v_status_t statuses[2];
    vi_msg_t vim;
    vii_msg_t msg;
    vti_desc_t *tx;
    v_status_t rc;

    rc = start_with_test_get();
    if (rc != V_STATUS_SUCCESS) {
        return fail("values init", rc);
    }
    fill_values(attrs, statuses, &vim, &local_state, local_buf, sizeof(local_buf));
    rc = vi_system_get(&vim);
    if (rc != V_STATUS_SUCCESS || attrs[1].value.text.len != TEST_COMMIT_LEN) {
        stop_with_test_get(NULL);
        return fail("values local get", rc);
    }

    tx = connect_link(link);
    if (tx == NULL) {
        stop_with_test_get(NULL);
        return fail("values connect", V_STATUS_FAILURE);
    }
    fill_values(attrs, statuses, &vim, &wire_state, wire_buf, sizeof(wire_buf));
    system_msg(&msg, &vim, V_VERB_GET);
    rc = vti_call(tx, &msg);
    stop_with_test_get(tx);
    if (rc != V_STATUS_SUCCESS || statuses[0] != V_STATUS_SUCCESS ||
        statuses[1] != V_STATUS_SUCCESS) {
        return fail("values wire get", rc);
    }
    if (wire_state.operational != VI_SYSTEM_STATE_READY ||
        wire_state.operational != local_state.operational) {
        return fail("values state", (v_status_t)wire_state.operational);
    }
    if (attrs[1].value.text.len != TEST_COMMIT_LEN ||
        memcmp(wire_buf, local_buf, TEST_COMMIT_LEN) != 0) {
        return fail("values commit", (v_status_t)attrs[1].value.text.len);
    }
    return 0;
}

/**
 * @brief Over @p link: a small text room overflows, an oversize text is refused, and the
 *        node still serves a normal get.
 *
 * @param[in] link VTI_LINK_IPC or VTI_LINK_REMOTE
 * @return 0 on success, 1 on failure.
 */
int vedam_vta_text_bounds(int link)
{
    char big[VTI_CODEC_VALUE_MAX + 1];
    char buf[TEST_TEXT_ROOM];
    vi_system_state_t state;
    v_attr_t attrs[2];
    v_status_t statuses[2];
    vi_msg_t vim;
    vii_msg_t msg;
    vti_desc_t *tx;
    v_status_t rc;
    int i;

    rc = start_with_test_get();
    if (rc != V_STATUS_SUCCESS) {
        return fail("bounds init", rc);
    }
    tx = connect_link(link);
    if (tx == NULL) {
        stop_with_test_get(NULL);
        return fail("bounds connect", V_STATUS_FAILURE);
    }

    memset(buf, 'x', sizeof(buf));
    fill_values(attrs, statuses, &vim, &state, buf, 8);
    system_msg(&msg, &vim, V_VERB_GET);
    rc = vti_call(tx, &msg);
    if (rc != V_STATUS_BUFFER_OVERFLOW || statuses[0] != V_STATUS_SUCCESS ||
        statuses[1] != V_STATUS_BUFFER_OVERFLOW || attrs[1].value.text.len != TEST_COMMIT_LEN) {
        stop_with_test_get(tx);
        return fail("bounds small room", rc);
    }
    for (i = 0; i < (int)sizeof(buf); i++) {
        if (buf[i] != 'x') {
            stop_with_test_get(tx);
            return fail("bounds wrote past room", (v_status_t)i);
        }
    }

    memset(big, 'a', sizeof(big));
    fill_values(attrs, statuses, &vim, &state, big, sizeof(big));
    vim.attr_list = &attrs[1];
    vim.status_list = &statuses[1];
    vim.attr_count = 1;
    system_msg(&msg, &vim, V_VERB_SET);
    rc = vti_call(tx, &msg);
    if (rc != V_STATUS_BUFFER_OVERFLOW || statuses[1] != V_STATUS_BUFFER_OVERFLOW) {
        stop_with_test_get(tx);
        return fail("bounds oversize request", rc);
    }

    fill_values(attrs, statuses, &vim, &state, buf, sizeof(buf));
    system_msg(&msg, &vim, V_VERB_GET);
    rc = vti_call(tx, &msg);
    stop_with_test_get(tx);
    if (rc != V_STATUS_SUCCESS || attrs[1].value.text.len != TEST_COMMIT_LEN) {
        return fail("bounds get after refusal", rc);
    }
    return 0;
}

/**
 * @brief Over @p link: a list fits its room, then a smaller room overflows with the needed count.
 *
 * @param[in] link VTI_LINK_IPC or VTI_LINK_REMOTE
 * @return 0 on success, 1 on failure.
 */
int vedam_vta_list(int link)
{
    vuint32_t items[TEST_LIST_MAX];
    v_attr_t attr;
    v_status_t status;
    vi_msg_t vim;
    vii_msg_t msg;
    vti_desc_t *tx;
    v_status_t rc;

    rc = start_with_test_get();
    if (rc != V_STATUS_SUCCESS) {
        return fail("list init", rc);
    }
    tx = connect_link(link);
    if (tx == NULL) {
        stop_with_test_get(NULL);
        return fail("list connect", V_STATUS_FAILURE);
    }

    memset(items, 0, sizeof(items));
    memset(&attr, 0, sizeof(attr));
    memset(&vim, 0, sizeof(vim));
    attr.id = TEST_LIST_ID;
    attr.value.list.items = items;
    attr.value.list.count = TEST_LIST_MAX;
    vim.attr_count = 1;
    vim.attr_list = &attr;
    vim.status_list = &status;
    system_msg(&msg, &vim, V_VERB_GET);
    rc = vti_call(tx, &msg);
    if (rc != V_STATUS_SUCCESS || attr.value.list.count != TEST_LIST_LEN || items[0] != 7 ||
        items[2] != 9 || items[3] != 0) {
        stop_with_test_get(tx);
        return fail("list fits", rc);
    }

    memset(items, 0, sizeof(items));
    attr.value.list.items = items;
    attr.value.list.count = 2;
    rc = vti_call(tx, &msg);
    stop_with_test_get(tx);
    if (rc != V_STATUS_BUFFER_OVERFLOW || status != V_STATUS_BUFFER_OVERFLOW ||
        attr.value.list.count != TEST_LIST_LEN || items[0] != 0) {
        return fail("list overflow", rc);
    }
    return 0;
}

/**
 * @brief Get RELEASE_COMMIT over @p link with the real System get body.
 *
 * The class table is not swapped. The body writes nothing, so the reply is
 * success with an empty text and the caller's buffer is left untouched.
 *
 * @param[in] link VTI_LINK_IPC or VTI_LINK_REMOTE
 * @return 0 on success, 1 on failure.
 */
int vedam_vta_empty_commit(int link)
{
    char buf[VTI_CODEC_VALUE_MAX];
    v_attr_t attr;
    v_status_t status = V_STATUS_SUCCESS;
    vi_msg_t vim;
    vii_msg_t msg;
    vti_desc_t *tx;
    v_status_t rc;
    int i;

    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        return fail("empty commit init", rc);
    }
    tx = connect_link(link);
    if (tx == NULL) {
        vedam_deinit();
        return fail("empty commit connect", V_STATUS_FAILURE);
    }

    memset(buf, 'x', sizeof(buf));
    memset(&attr, 0, sizeof(attr));
    memset(&vim, 0, sizeof(vim));
    attr.id = VI_SYSTEM_ATTR_RELEASE_COMMIT;
    attr.value.text.buf = buf;
    attr.value.text.len = sizeof(buf);
    vim.attr_count = 1;
    vim.attr_list = &attr;
    vim.status_list = &status;
    system_msg(&msg, &vim, V_VERB_GET);
    rc = vti_call(tx, &msg);
    vti_close(tx);
    vedam_deinit();
    if (rc != V_STATUS_SUCCESS || status != V_STATUS_SUCCESS || attr.value.text.len != 0) {
        return fail("empty commit", rc != V_STATUS_SUCCESS ? rc : (v_status_t)attr.value.text.len);
    }
    for (i = 0; i < VTI_CODEC_VALUE_MAX; i++) {
        if (buf[i] != 'x') {
            return fail("empty commit wrote bytes", (v_status_t)i);
        }
    }
    return 0;
}

/**
 * @brief Over @p link, a get that writes exactly one slot returns all of those bytes.
 *
 * @param[in] link VTI_LINK_IPC or VTI_LINK_REMOTE
 * @return 0 on success, 1 on failure.
 */
int vedam_vta_full_text(int link)
{
    char buf[VTI_CODEC_VALUE_MAX];
    v_attr_t attr;
    v_status_t status = V_STATUS_SUCCESS;
    vi_msg_t vim;
    vii_msg_t msg;
    vti_desc_t *tx;
    v_status_t rc;

    prepare_full_text();
    rc = start_with_test_get();
    if (rc != V_STATUS_SUCCESS) {
        return fail("full text init", rc);
    }
    test_classes[V_CLASS_SYSTEM].body[V_VERB_GET] = test_system_get_full;

    tx = connect_link(link);
    if (tx == NULL) {
        stop_with_test_get(NULL);
        return fail("full text connect", V_STATUS_FAILURE);
    }
    memset(buf, 0, sizeof(buf));
    memset(&attr, 0, sizeof(attr));
    memset(&vim, 0, sizeof(vim));
    attr.id = VI_SYSTEM_ATTR_RELEASE_COMMIT;
    attr.value.text.buf = buf;
    attr.value.text.len = sizeof(buf);
    vim.attr_count = 1;
    vim.attr_list = &attr;
    vim.status_list = &status;
    system_msg(&msg, &vim, V_VERB_GET);
    rc = vti_call(tx, &msg);
    stop_with_test_get(tx);
    if (rc != V_STATUS_SUCCESS || status != V_STATUS_SUCCESS ||
        attr.value.text.len != VTI_CODEC_VALUE_MAX ||
        memcmp(buf, full_text, VTI_CODEC_VALUE_MAX) != 0) {
        return fail("full text", rc != V_STATUS_SUCCESS ? rc : (v_status_t)attr.value.text.len);
    }
    return 0;
}

/**
 * @brief Run Round 8 codec path checks.
 *
 * @return 0 on success, 1 on failure.
 */
int vedam_legacy_codec(void)
{
    if (test_local_skips_codec() != 0) {
        return 1;
    }
#if VEDAM_ENABLE_MQ
    if (test_mq_mixed() != 0) {
        return 1;
    }
#endif
#if VEDAM_ENABLE_GRPC
    if (test_http_mixed() != 0) {
        return 1;
    }
    if (test_http_one_socket() != 0) {
        return 1;
    }
#endif
    return 0;
}

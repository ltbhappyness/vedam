/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * Lightweight helpers that do not need a live System singleton.
 */
#include "util/attr.h"
#include "vi/class/system.h"
#include "vii/class/system.h"
#include "vii/meta.h"

#include <stdio.h>
#include <string.h>

static int fail(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
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

static int test_attr_collapse(void)
{
    v_attr_t attrs[4];
    vuint32_t n;

    attrs[0].id = VI_SYSTEM_ATTR_NODE_CLASS;
    attrs[0].value.s32 = 1;
    attrs[1].id = VI_SYSTEM_ATTR_PROTOCOL_VERSION;
    attrs[2].id = VI_SYSTEM_ATTR_NODE_CLASS;
    attrs[2].value.s32 = 3;
    attrs[3].id = VI_SYSTEM_ATTR_SYSTEM_STATE;

    n = vattr_collapse_last(attrs, 4);
    if (n != 3) {
        return fail("attr collapse count");
    }
    if (attrs[0].id != VI_SYSTEM_ATTR_PROTOCOL_VERSION) {
        return fail("attr collapse order");
    }
    if (attrs[1].id != VI_SYSTEM_ATTR_NODE_CLASS || attrs[1].value.s32 != 3) {
        return fail("attr collapse last wins");
    }
    if (attrs[2].id != VI_SYSTEM_ATTR_SYSTEM_STATE) {
        return fail("attr collapse tail");
    }
    return 0;
}

static int test_via_before_init(void)
{
    vi_msg_t vim;
    v_status_t rc;

    memset(&vim, 0, sizeof(vim));
    rc = system_verb(V_ORIGIN_INTERNAL, V_VERB_CREATE, &vim);
    if (rc != V_STATUS_SUCCESS) {
        fprintf(stderr, "via create stub %d\n", (int)rc);
        return 1;
    }
    return 0;
}

int vedam_legacy_util(void)
{
    if (test_attr_collapse() != 0) {
        return 1;
    }
    if (test_via_before_init() != 0) {
        return 1;
    }
    return 0;
}

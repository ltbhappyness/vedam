/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "util/attr.h"
#include "vii/meta.h"

/**
 * @brief Require the lists, collapse when the verb asks, and check every attribute.
 *
 * @param[in]     cls    Class row
 * @param[in]     origin Call origin
 * @param[in,out] msg    Call message
 * @param[in]     verb   Verb being checked
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or the first contract failure.
 */
static v_status_t check_list(const struct vii_class *cls, v_origin_t origin, vi_msg_t *msg,
                             v_verb_t verb)
{
    if (msg == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (!vii_verb_rules[verb].checks_list) {
        return V_STATUS_SUCCESS;
    }
    if (msg->attr_count != 0 && (msg->attr_list == NULL || msg->status_list == NULL)) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (vii_verb_rules[verb].collapse) {
        msg->attr_count = vattr_collapse_last(msg->attr_list, msg->attr_count);
    }
    return vii_attr_check(cls, origin, msg, verb);
}

/**
 * @brief Run one verb of @p cls: origin gate, collapse, checks, then the library body.
 *
 * @param[in]     cls  Class row
 * @param[in,out] call Internal call
 * @return The library status, or the first contract failure.
 */
v_status_t vii_verb(const struct vii_class *cls, vii_msg_t *call)
{
    vii_body_fn body;
    v_status_t st;

    if ((cls->external_verbs & (1u << call->verb)) == 0 && call->origin != V_ORIGIN_INTERNAL) {
        return V_STATUS_NOT_IMPLEMENTED;
    }
    body = cls->body[call->verb];
    if (body == NULL) {
        return V_STATUS_NOT_IMPLEMENTED;
    }

    st = check_list(cls, call->origin, call->msg, call->verb);
    if (st != V_STATUS_SUCCESS) {
        return st;
    }
    /* On subscribe the body copies what it keeps. The caller's list stays owned by the caller. */
    return body(call->major, call->minor, call->msg);
}

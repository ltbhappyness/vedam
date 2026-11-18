/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "vii/dispatch.h"

#include "core/node.h"
#include "lock/store.h"
#include "vii/meta.h"

/**
 * @brief Run the class verb for @p user once the locks for the call are held.
 *
 * @param[in,out] user The vii_msg_t passed to vii_dispatch
 * @return The verb status, or V_STATUS_NOT_IMPLEMENTED when the class or verb is unknown.
 */
static v_status_t dispatch_body(void *user)
{
    vii_msg_t *call = user;
    vedam_t *node = vedam_node();

    if (call->class_id == V_CLASS_UNSPECIFIED ||
        (unsigned)call->class_id > (unsigned)V_CLASS_ID_MAX) {
        return V_STATUS_NOT_IMPLEMENTED;
    }
    if ((unsigned)call->verb >= (unsigned)V_VERB_COUNT) {
        return V_STATUS_NOT_IMPLEMENTED;
    }
    return vii_verb(&node->classes[call->class_id], call);
}

/**
 * @brief Reject a dispatch that has no node or no message.
 *
 * @param[in] call Internal call
 * @return V_STATUS_SUCCESS, V_STATUS_UNINITIALIZED, or V_STATUS_INVALID_PARAMETER.
 */
static v_status_t dispatch_ready(vii_msg_t *call)
{
    if (vedam_node() == NULL) {
        return V_STATUS_UNINITIALIZED;
    }
    if (call == NULL || call->msg == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    return V_STATUS_SUCCESS;
}

#if VEDAM_ENABLE_WWLOCK

/**
 * @brief Enter the node for @p call under the Wait-Die lock map.
 *
 * Local vi_*, init, and a transport receive that already holds a vi_msg_t
 * call this. vedam_lock_invoke takes the locks, then the class verb runs.
 * A library that needs another vertical calls this again with internal
 * origin. That call joins the attempt already running on the thread. A
 * Wait-Die die returns VEDAM_LOCK_RESTART and the outermost attempt starts
 * again. A send uses vedam_node_call after that attempt releases its locks.
 *
 * @param[in,out] call Internal call (class, verb, origin, version, and vi_msg_t)
 * @return The verb status, or V_STATUS_NOT_IMPLEMENTED when the class or verb is unknown.
 */
v_status_t vii_dispatch(vii_msg_t *call)
{
    vi_msg_t *msg;
    v_status_t st;

    st = dispatch_ready(call);
    if (st != V_STATUS_SUCCESS) {
        return st;
    }
    msg = call->msg;
    return vedam_lock_invoke(call->class_id, msg->key, msg->key_len, call->verb, call->origin,
                             msg->attr_count, msg->attr_list, msg->status_list, dispatch_body,
                             call);
}

#else

static int dispatch_depth;

/**
 * @brief Enter the node for @p call on this thread and run the class verb.
 *
 * A second entry before the first returns V_STATUS_FAILED_PRECONDITION.
 *
 * @param[in,out] call Internal call (class, verb, origin, version, and vi_msg_t)
 * @return The verb status, V_STATUS_FAILED_PRECONDITION on re-entry, or
 *         V_STATUS_NOT_IMPLEMENTED when the class or verb is unknown.
 */
v_status_t vii_dispatch(vii_msg_t *call)
{
    v_status_t st;

    st = dispatch_ready(call);
    if (st != V_STATUS_SUCCESS) {
        return st;
    }
    if (dispatch_depth != 0) {
        return V_STATUS_FAILED_PRECONDITION;
    }
    dispatch_depth++;
    st = dispatch_body(call);
    dispatch_depth--;
    return st;
}

#endif /* !VEDAM_ENABLE_WWLOCK */

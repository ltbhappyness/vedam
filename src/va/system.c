/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#include "vi/class/system.h"
#include "vii/class/system.h"
#include "vii/dispatch.h"

/**
 * @brief Build the internal get call for system and dispatch it.
 *
 * @param[in,out] msg Call message
 * @return The dispatch status.
 */
v_status_t vi_system_get(vi_msg_t *msg)
{
    vii_msg_t call;

    if (msg == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    msg->key = NULL;
    msg->key_len = 0;
    call.class_id = V_CLASS_SYSTEM;
    call.origin = V_ORIGIN_EXTERNAL;
    call.verb = V_VERB_GET;
    call.major = VII_SYSTEM_MAJOR;
    call.minor = VII_SYSTEM_MINOR;
    call.msg = msg;
    return vii_dispatch(&call);
}

/**
 * @brief Build the internal subscribe call for system and dispatch it.
 *
 * @param[in,out] msg Call message
 * @return The dispatch status.
 */
v_status_t vi_system_subscribe(vi_msg_t *msg)
{
    vii_msg_t call;

    if (msg == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    msg->key = NULL;
    msg->key_len = 0;
    call.class_id = V_CLASS_SYSTEM;
    call.origin = V_ORIGIN_EXTERNAL;
    call.verb = V_VERB_SUBSCRIBE;
    call.major = VII_SYSTEM_MAJOR;
    call.minor = VII_SYSTEM_MINOR;
    call.msg = msg;
    return vii_dispatch(&call);
}

/**
 * @brief Build the internal unsubscribe call for system and dispatch it.
 *
 * @param[in,out] msg Call message
 * @return The dispatch status.
 */
v_status_t vi_system_unsubscribe(vi_msg_t *msg)
{
    vii_msg_t call;

    if (msg == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    msg->key = NULL;
    msg->key_len = 0;
    call.class_id = V_CLASS_SYSTEM;
    call.origin = V_ORIGIN_EXTERNAL;
    call.verb = V_VERB_UNSUBSCRIBE;
    call.major = VII_SYSTEM_MAJOR;
    call.minor = VII_SYSTEM_MINOR;
    call.msg = msg;
    return vii_dispatch(&call);
}

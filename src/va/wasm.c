/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#include "vi/class/wasm.h"
#include "vii/class/wasm.h"
#include "vii/dispatch.h"

/**
 * @brief Build the internal get call for wasm and dispatch it.
 *
 * @param[in,out] msg Call message
 * @return The dispatch status.
 */
v_status_t vi_wasm_get(vi_msg_t *msg)
{
    vii_msg_t call;

    if (msg == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    msg->key = NULL;
    msg->key_len = 0;
    call.class_id = V_CLASS_WASM;
    call.origin = V_ORIGIN_EXTERNAL;
    call.verb = V_VERB_GET;
    call.major = VII_WASM_MAJOR;
    call.minor = VII_WASM_MINOR;
    call.msg = msg;
    return vii_dispatch(&call);
}

/**
 * @brief Build the internal subscribe call for wasm and dispatch it.
 *
 * @param[in,out] msg Call message
 * @return The dispatch status.
 */
v_status_t vi_wasm_subscribe(vi_msg_t *msg)
{
    vii_msg_t call;

    if (msg == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    msg->key = NULL;
    msg->key_len = 0;
    call.class_id = V_CLASS_WASM;
    call.origin = V_ORIGIN_EXTERNAL;
    call.verb = V_VERB_SUBSCRIBE;
    call.major = VII_WASM_MAJOR;
    call.minor = VII_WASM_MINOR;
    call.msg = msg;
    return vii_dispatch(&call);
}

/**
 * @brief Build the internal unsubscribe call for wasm and dispatch it.
 *
 * @param[in,out] msg Call message
 * @return The dispatch status.
 */
v_status_t vi_wasm_unsubscribe(vi_msg_t *msg)
{
    vii_msg_t call;

    if (msg == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    msg->key = NULL;
    msg->key_len = 0;
    call.class_id = V_CLASS_WASM;
    call.origin = V_ORIGIN_EXTERNAL;
    call.verb = V_VERB_UNSUBSCRIBE;
    call.major = VII_WASM_MAJOR;
    call.minor = VII_WASM_MINOR;
    call.msg = msg;
    return vii_dispatch(&call);
}

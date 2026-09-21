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

#include "vti/dispatch.h"

/**
 * @brief Public get for wasm. Enters vii through the local transport.
 *
 * @param[in,out] msg Call message
 * @return The vii status.
 */
v_status_t vi_wasm_get(vi_msg_t *msg)
{
    vta_msg_t call;

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
    return vta_dispatch(&call);
}

/**
 * @brief Public subscribe for wasm. Enters vii through the local transport.
 *
 * @param[in,out] msg Call message
 * @return The vii status.
 */
v_status_t vi_wasm_subscribe(vi_msg_t *msg)
{
    vta_msg_t call;

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
    return vta_dispatch(&call);
}

/**
 * @brief Public unsubscribe for wasm. Enters vii through the local transport.
 *
 * @param[in,out] msg Call message
 * @return The vii status.
 */
v_status_t vi_wasm_unsubscribe(vi_msg_t *msg)
{
    vta_msg_t call;

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
    return vta_dispatch(&call);
}

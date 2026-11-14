/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * QUIC stream is the descriptor once a handshake exists. This slice does not
 * bind a socket. It does not forward into the local receiver.
 */
#include "vti/connect.h"

#include "util/log.h"
#include "util/trace.h"

/**
 * @brief Close a Connect descriptor (no socket in this slice).
 * @param[in] desc Argument desc
 * @return Result of conn_close.
 */
static v_status_t conn_close(vti_desc_t *desc)
{
    (void)desc;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Mark Connect listen as bound without opening a socket.
 * @param[in] desc Argument desc
 * @param[in] address Argument address
 * @return Result of conn_listen.
 */
static v_status_t conn_listen(vti_desc_t *desc, const char *address)
{
    (void)desc;
    (void)address;
    vtrace_enter();
    vlog_info("connect listen is not bound in this slice");
    vtrace_exit();
    return V_STATUS_SUCCESS;
}

/**
 * @brief Accept is not implemented for Connect in this slice.
 * @param[in] desc Argument desc
 * @param[in] client Argument client
 * @return Result of conn_accept.
 */
static v_status_t conn_accept(vti_desc_t *desc, vti_desc_t **client)
{
    (void)desc;
    (void)client;
    return V_STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Connect dial is not implemented in this slice.
 * @param[in] desc Argument desc
 * @param[in] address Argument address
 * @return Result of conn_connect.
 */
static v_status_t conn_connect(vti_desc_t *desc, const char *address)
{
    (void)desc;
    (void)address;
    return V_STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Client call over Connect is not implemented in this slice.
 *
 * @param[in]     desc Transport descriptor
 * @param[in,out] msg  Call message
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
static v_status_t conn_call(vti_desc_t *desc, vii_msg_t *msg)
{
    (void)desc;
    (void)msg;
    return V_STATUS_NOT_IMPLEMENTED;
}

const vti_ops_t vti_connect_ops = {
    .close = conn_close,
    .listen = conn_listen,
    .accept = conn_accept,
    .connect = conn_connect,
    .call = conn_call,
};

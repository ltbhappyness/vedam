/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * The caller's thread is the connection. There is no accept thread.
 */
#include "vti/dispatch.h"
#include "vti/inproc.h"

#include "platform/platform.h"
#include "util/log.h"
#include "util/trace.h"

struct inproc_ctx {
    int listening;
};

/**
 * @brief Free the in-process descriptor context.
 *
 * @param[in] desc Transport descriptor
 * @return V_STATUS_SUCCESS.
 */
static v_status_t inproc_close(vta_desc_t *desc)
{
    vtrace_enter();
    vplatform_free(desc->ctx);
    desc->ctx = NULL;
    vtrace_exit();
    return V_STATUS_SUCCESS;
}

/**
 * @brief Mark the descriptor as the local listen door.
 *
 * @param[in] desc    Transport descriptor
 * @param[in] address Unused for in-process
 * @return V_STATUS_SUCCESS or V_STATUS_NO_MEMORY.
 */
static v_status_t inproc_listen(vta_desc_t *desc, const char *address)
{
    struct inproc_ctx *ctx;

    (void)address;
    ctx = (struct inproc_ctx *)vplatform_calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    ctx->listening = 1;
    desc->ctx = ctx;
    vlog_info("inproc listen stays on the caller thread");
    return V_STATUS_SUCCESS;
}

/**
 * @brief Accept is unused for in-process; the caller thread is the connection.
 *
 * @param[in]  desc   Listen descriptor
 * @param[out] client Unused
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
static v_status_t inproc_accept(vta_desc_t *desc, vta_desc_t **client)
{
    (void)desc;
    (void)client;
    return V_STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Open an in-process sender context on @p desc.
 *
 * @param[in] desc    Transport descriptor
 * @param[in] address Unused for in-process
 * @return V_STATUS_SUCCESS or V_STATUS_NO_MEMORY.
 */
static v_status_t inproc_connect(vta_desc_t *desc, const char *address)
{
    (void)address;
    desc->ctx = vplatform_calloc(1, sizeof(struct inproc_ctx));
    if (desc->ctx == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    return V_STATUS_SUCCESS;
}

/**
 * @brief Serve @p msg on the local door through vta_dispatch.
 *
 * @param[in]     desc Transport descriptor
 * @param[in,out] msg  Call and reply message
 * @return The dispatch status.
 */
static v_status_t inproc_call(vta_desc_t *desc, vta_msg_t *msg)
{
    (void)desc;
    return vta_dispatch(msg);
}

const vta_ops_t vta_inproc_ops = {
    .close = inproc_close,
    .listen = inproc_listen,
    .accept = inproc_accept,
    .connect = inproc_connect,
    .call = inproc_call,
};

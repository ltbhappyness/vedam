/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "vti/grpc.h"
#include "vti/grpc_uds.h"

#include "platform/platform.h"
#include "util/log.h"
#include "util/trace.h"

#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

struct grpc_uds_ctx {
    struct vta_http_pool pool;
    int fd;
    int running;
    char path[108];
};

/**
 * @brief Stop the Unix-socket listen path and free its context.
 * @param[in] desc Argument desc
 * @return Result of uds_close.
 */
static v_status_t uds_close(vta_desc_t *desc)
{
    struct grpc_uds_ctx *c = (struct grpc_uds_ctx *)desc->ctx;

    if (c == NULL) {
        return V_STATUS_SUCCESS;
    }
    if (c->running) {
        vta_http_pool_stop(&c->pool);
        c->running = 0;
    }
    vta_grpc_unbind(c->fd, c->path);
    c->fd = -1;
    vplatform_free(c);
    desc->ctx = NULL;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Create, bind, and listen on a Unix-domain socket path.
 * @param[in] path Argument path
 * @return Result of bind_uds.
 */
static int bind_uds(const char *path)
{
    int fd;
    int rc;
    struct sockaddr_un addr;

    if (path == NULL || path[0] == '\0') {
        return -1;
    }
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    vplatform_memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    vplatform_strlcpy(addr.sun_path, path, sizeof(addr.sun_path));

    rc = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc < 0) {
        goto out_close;
    }

    rc = listen(fd, 16);
    if (rc < 0) {
        goto out_close;
    }
    return fd;

out_close:
    close(fd);
    return -1;
}

/**
 * @brief Bind a Unix-domain socket and start the HTTP worker pool.
 * @param[in] desc Argument desc
 * @param[in] address Argument address
 * @return Result of uds_listen.
 */
static v_status_t uds_listen(vta_desc_t *desc, const char *address)
{
    struct grpc_uds_ctx *c;
    int rc;

    if (address == NULL || address[0] == '\0') {
        vlog_error("grpc_uds address is empty");
        return V_STATUS_INVALID_PARAMETER;
    }
    c = vplatform_calloc(1, sizeof(*c));
    if (c == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    c->fd = -1;
    desc->ctx = c;
    vtrace_enter();
    vplatform_strlcpy(c->path, address, sizeof(c->path));

    c->fd = bind_uds(c->path);
    if (c->fd < 0) {
        vlog_error("grpc_uds bind failed");
        goto out_free;
    }

    rc = vta_http_pool_start(&c->pool, c->fd);
    if (rc != 0) {
        goto out_unbind;
    }
    c->running = 1;
    vlog_info("grpc_uds listen %s", c->path);
    vtrace_exit();
    return V_STATUS_SUCCESS;

out_unbind:
    vta_grpc_unbind(c->fd, c->path);
    c->fd = -1;
out_free:
    vplatform_free(c);
    desc->ctx = NULL;
    return V_STATUS_FAILURE;
}

/**
 * @brief Accept is unused; the HTTP pool owns connections.
 * @param[in] desc Argument desc
 * @param[in] client Argument client
 * @return Result of uds_accept.
 */
static v_status_t uds_accept(vta_desc_t *desc, vta_desc_t **client)
{
    (void)desc;
    (void)client;
    return V_STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Client connect over the Unix-socket door is not implemented in this slice.
 * @param[in] desc Argument desc
 * @param[in] address Argument address
 * @return Result of uds_connect.
 */
static v_status_t uds_connect(vta_desc_t *desc, const char *address)
{
    (void)desc;
    (void)address;
    return V_STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Client call over the Unix-socket door is not implemented in this slice.
 *
 * @param[in]     desc Transport descriptor
 * @param[in,out] msg  Call message
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
static v_status_t uds_call(vta_desc_t *desc, vta_msg_t *msg)
{
    (void)desc;
    (void)msg;
    return V_STATUS_NOT_IMPLEMENTED;
}

const vta_ops_t vta_grpc_uds_ops = {
    .close = uds_close,
    .listen = uds_listen,
    .accept = uds_accept,
    .connect = uds_connect,
    .call = uds_call,
};

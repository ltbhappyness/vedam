/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * Listen owns a fixed pool of workers on this descriptor. Close wakes the
 * pool and joins it before the context is freed. A connected descriptor
 * forwards one vi_msg_t per call without holding the object lock.
 */
#include "vti/grpc.h"

#if VEDAM_ENABLE_GRPC

#include "platform/platform.h"
#include "util/log.h"
#include "util/trace.h"
#include "vti/class_uuid.h"
#include "vti/codec.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct grpc_ctx {
    struct vti_http_pool pool;
    int fd;
    int running;
    int connected;
    unsigned tcp_connects;
    char bind[128];
    uint8_t enc[VTI_CODEC_BUF_SIZE];
    uint8_t http_io[VTI_HTTP_IO_MAX];
    vti_codec_ws_t *codec;
};

/**
 * @brief Stop the HTTP worker pool and free the gRPC listen context.
 *
 * @param[in,out] desc Transport descriptor
 * @return V_STATUS_SUCCESS.
 */
static v_status_t grpc_close(vti_desc_t *desc)
{
    struct grpc_ctx *c = (struct grpc_ctx *)desc->ctx;
    int i;

    if (c == NULL) {
        return V_STATUS_SUCCESS;
    }
    if (c->running) {
        vti_http_pool_stop(&c->pool);
        c->running = 0;
        for (i = 0; i < VTI_HTTP_WORKERS; i++) {
            vti_codec_ws_destroy(c->pool.slot[i].codec.ws);
            c->pool.slot[i].codec.ws = NULL;
        }
    }
    if (c->fd >= 0) {
        if (c->connected) {
            close(c->fd);
        } else {
            vti_grpc_unbind(c->fd, NULL);
        }
        c->fd = -1;
    }
    vti_codec_ws_destroy(c->codec);
    c->codec = NULL;
    vplatform_free(c);
    desc->ctx = NULL;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Bind TCP and start the HTTP worker pool.
 *
 * @param[in,out] desc    Transport descriptor
 * @param[in]     address Bind address
 * @return V_STATUS_SUCCESS, or a failure code.
 */
static v_status_t grpc_listen(vti_desc_t *desc, const char *address)
{
    struct grpc_ctx *c;
    int rc;
    int i;

    if (address == NULL || address[0] == '\0') {
        vlog_error("grpc address is empty");
        return V_STATUS_INVALID_PARAMETER;
    }
    c = vplatform_calloc(1, sizeof(*c));
    if (c == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    c->fd = -1;
    desc->ctx = c;
    vtrace_enter();
    vplatform_strlcpy(c->bind, address, sizeof(c->bind));

    c->fd = vti_grpc_bind_tcp(c->bind, c->bind, sizeof(c->bind));
    if (c->fd < 0) {
        goto out_free;
    }

    rc = vti_http_pool_start(&c->pool, c->fd);
    if (rc != 0) {
        goto out_unbind;
    }
    for (i = 0; i < VTI_HTTP_WORKERS; i++) {
        c->pool.slot[i].codec.ws = vti_codec_ws_create();
        if (c->pool.slot[i].codec.ws == NULL) {
            goto out_pool;
        }
    }
    c->running = 1;
    vlog_info("grpc listen %s", c->bind);
    vtrace_exit();
    return V_STATUS_SUCCESS;

out_pool:
    for (i = 0; i < VTI_HTTP_WORKERS; i++) {
        vti_codec_ws_destroy(c->pool.slot[i].codec.ws);
        c->pool.slot[i].codec.ws = NULL;
    }
    vti_http_pool_stop(&c->pool);
out_unbind:
    vti_grpc_unbind(c->fd, NULL);
    c->fd = -1;
out_free:
    vplatform_free(c);
    desc->ctx = NULL;
    return V_STATUS_FAILURE;
}

/**
 * @brief Accept is unused; the HTTP pool owns connections.
 *
 * @param[in]  desc   Listen descriptor
 * @param[out] client Unused
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
static v_status_t grpc_accept(vti_desc_t *desc, vti_desc_t **client)
{
    (void)desc;
    (void)client;
    return V_STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Parse host:port from @p address into @p host and @p port.
 *
 * @param[in]  address Host:port string
 * @param[out] host    Host buffer
 * @param[in]  host_cap Capacity of @p host
 * @param[out] port    Port number
 * @return V_STATUS_SUCCESS, or V_STATUS_INVALID_PARAMETER.
 */
static v_status_t parse_host_port(const char *address, char *host, size_t host_cap,
                                  unsigned long *port)
{
    const char *colon;
    char *end;
    size_t host_len;

    if (address == NULL || host == NULL || port == NULL || host_cap == 0) {
        return V_STATUS_INVALID_PARAMETER;
    }
    colon = strrchr(address, ':');
    if (colon == NULL || colon == address) {
        return V_STATUS_INVALID_PARAMETER;
    }
    host_len = (size_t)(colon - address);
    if (host_len >= host_cap) {
        return V_STATUS_INVALID_PARAMETER;
    }
    vplatform_memcpy(host, address, host_len);
    host[host_len] = '\0';
    *port = strtoul(colon + 1, &end, 10);
    if (end == colon + 1 || *end != '\0' || *port == 0 || *port > 65535ul) {
        return V_STATUS_INVALID_PARAMETER;
    }
    return V_STATUS_SUCCESS;
}

/**
 * @brief Open a TCP connection to the address stored on @p c.
 *
 * @param[in,out] c Client context
 * @return V_STATUS_SUCCESS, or a failure code.
 */
static v_status_t grpc_tcp_connect(struct grpc_ctx *c)
{
    struct sockaddr_in addr;
    char host[128];
    unsigned long port;
    v_status_t st;
    int fd;
    int rc;

    st = parse_host_port(c->bind, host, sizeof(host), &port);
    if (st != V_STATUS_SUCCESS) {
        return st;
    }

    if (c->fd >= 0) {
        close(c->fd);
        c->fd = -1;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return V_STATUS_FAILURE;
    }

    vplatform_memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    rc = inet_pton(AF_INET, host, &addr.sin_addr);
    if (rc != 1) {
        close(fd);
        return V_STATUS_INVALID_PARAMETER;
    }

    rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc != 0) {
        close(fd);
        return V_STATUS_FAILURE;
    }
    c->fd = fd;
    c->connected = 1;
    c->tcp_connects++;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Connect a TCP client to a temporary HTTP remote listener.
 *
 * @param[in,out] desc    Transport descriptor
 * @param[in]     address Host:port string
 * @return V_STATUS_SUCCESS, or a failure code.
 */
static v_status_t grpc_connect(vti_desc_t *desc, const char *address)
{
    struct grpc_ctx *c;
    v_status_t st;

    if (address == NULL || address[0] == '\0') {
        return V_STATUS_INVALID_PARAMETER;
    }

    c = vplatform_calloc(1, sizeof(*c));
    if (c == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    c->fd = -1;
    vplatform_strlcpy(c->bind, address, sizeof(c->bind));

    c->codec = vti_codec_ws_create();
    if (c->codec == NULL) {
        vplatform_free(c);
        return V_STATUS_NO_MEMORY;
    }

    st = grpc_tcp_connect(c);
    if (st != V_STATUS_SUCCESS) {
        vti_codec_ws_destroy(c->codec);
        vplatform_free(c);
        return st;
    }
    desc->ctx = c;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Encode @p msg, exchange it over HTTP, and decode into the same message.
 *
 * Uses the TCP socket opened at connect. Does not hold the object lock across
 * the write or the wait.
 *
 * @param[in]     desc Connected transport descriptor
 * @param[in,out] msg  Call and reply message
 * @return Overall status from the peer, or a local failure code.
 */
static v_status_t grpc_call(vti_desc_t *desc, vii_msg_t *msg)
{
    struct grpc_ctx *c;
    const char *verb_name;
    const uint8_t *resp;
    const char *service;
    size_t enc_len;
    size_t frame_len;
    size_t resp_len;
    v_status_t st;
    v_status_t overall;

    if (desc == NULL || desc->ctx == NULL || msg == NULL || msg->msg == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    c = (struct grpc_ctx *)desc->ctx;
    if (!c->connected || c->fd < 0 || c->bind[0] == '\0' || c->codec == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (msg->msg->attr_count > 0 &&
        (msg->msg->attr_list == NULL || msg->msg->status_list == NULL)) {
        return V_STATUS_INVALID_PARAMETER;
    }

    verb_name = vti_codec_verb_name(msg->verb);
    if (verb_name == NULL) {
        return V_STATUS_NOT_IMPLEMENTED;
    }

    service = vti_service_name(msg->class_id, msg->major);
    if (service == NULL) {
        return V_STATUS_NOT_IMPLEMENTED;
    }

    st = vti_codec_encode(msg, V_STATUS_SUCCESS, 0, c->enc + 5, sizeof(c->enc) - 5u, &enc_len,
                          c->codec);
    if (st != V_STATUS_SUCCESS) {
        return st;
    }

    st = vti_codec_grpc_frame(c->enc, sizeof(c->enc), enc_len, &frame_len);
    if (st != V_STATUS_SUCCESS) {
        return st;
    }

    st = vti_grpc_http_exchange(c->fd, service, verb_name, c->enc, frame_len, &resp, &resp_len,
                                c->http_io, sizeof(c->http_io));
    if (st != V_STATUS_SUCCESS) {
        return st;
    }

    st = vti_codec_decode(resp, resp_len, msg, &overall, c->codec, NULL);
    if (st != V_STATUS_SUCCESS) {
        return st;
    }
    return overall;
}

/**
 * @brief Return the bound TCP address string for a listen descriptor.
 *
 * @param[in] desc Listen descriptor
 * @return Bound address, or NULL.
 */
const char *vti_grpc_bound(vti_desc_t *desc)
{
    struct grpc_ctx *c;

    if (desc == NULL || desc->ctx == NULL) {
        return NULL;
    }
    c = (struct grpc_ctx *)desc->ctx;
    if (c->fd < 0 || c->connected) {
        return NULL;
    }
    return c->bind;
}

/**
 * @brief Number of TCP connect syscalls performed for a connected descriptor.
 *
 * @param[in] desc Connected remote descriptor
 * @return Connect count, or 0 when @p desc is not a connected client.
 */
unsigned vti_grpc_tcp_connects(vti_desc_t *desc)
{
    struct grpc_ctx *c;

    if (desc == NULL || desc->ctx == NULL) {
        return 0;
    }
    c = (struct grpc_ctx *)desc->ctx;
    if (!c->connected) {
        return 0;
    }
    return c->tcp_connects;
}

const vti_ops_t vti_grpc_ops = {
    .close = grpc_close,
    .listen = grpc_listen,
    .accept = grpc_accept,
    .connect = grpc_connect,
    .call = grpc_call,
};

#endif /* VEDAM_ENABLE_GRPC */

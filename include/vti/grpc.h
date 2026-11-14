/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VTI_GRPC_H
#define VTI_GRPC_H

#include "platform/thread.h"
#include "vti/codec.h"
#include "vti/vti.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { VTI_HTTP_WORKERS = 4, VTI_HTTP_IO_MAX = 65536 };

struct vti_http_pool;

struct vti_http_slot {
    struct vti_http_pool *pool;
    int index;
    int fd;
    int kick_r;
    int kick_w;
    int busy;
    int started;
    vplatform_thread_t thread;
    uint8_t out_buf[VTI_CODEC_BUF_SIZE];
    uint8_t http_buf[VTI_HTTP_IO_MAX];
    struct vti_codec_mem codec;
};

struct vti_http_pool {
    int listen_fd;
    int stop;
    int poke_r;
    int poke_w;
    int listen_started;
    vplatform_thread_t listen_thread;
    struct vti_http_slot slot[VTI_HTTP_WORKERS];
};

extern const vti_ops_t vti_grpc_ops;

int vti_grpc_bind_tcp(const char *bind_spec, char *bound, size_t bound_cap);
int vti_http_pool_start(struct vti_http_pool *pool, int listen_fd);
void vti_http_pool_stop(struct vti_http_pool *pool);
void vti_grpc_unbind(int fd, const char *uds_path);

/**
 * @brief Address string the listening descriptor bound, including the kernel port.
 */
const char *vti_grpc_bound(vti_desc_t *desc);

/**
 * @brief Number of TCP connect syscalls performed for a connected descriptor.
 *
 * Used by tests to show keep-alive: two calls share one socket.
 *
 * @param[in] desc Connected remote descriptor
 * @return Connect count, or 0 when @p desc is not a connected client.
 */
unsigned vti_grpc_tcp_connects(vti_desc_t *desc);

/**
 * @brief Send one framed protobuf Message and read the framed reply.
 *
 * The path is /<package>.<Service>/<Method>, for example
 * /vedam.system.v1.SystemService/Get, the path a stock gRPC client uses.
 * @p req already carries the five-byte grpc-web header. The reply body is
 * unframed in place inside @p io_buf; *@p resp points at those bytes.
 *
 * Reads the response using Content-Length and leaves the socket open.
 *
 * @param[in]     fd         Connected TCP socket
 * @param[in]     service    Service name, such as vedam.system.v1.SystemService
 * @param[in]     verb_name  Method name
 * @param[in]     req        Framed request bytes (header plus Message)
 * @param[in]     req_len    Length of @p req
 * @param[out]    resp       Points at unframed Message bytes inside @p io_buf
 * @param[out]    resp_len   Length of the unframed Message
 * @param[in,out] io_buf     Connection-owned HTTP scratch buffer
 * @param[in]     io_cap     Capacity of @p io_buf
 * @return V_STATUS_SUCCESS, or a failure code.
 */
v_status_t vti_grpc_http_exchange(int fd, const char *service, const char *verb_name,
                                  const uint8_t *req, size_t req_len, const uint8_t **resp,
                                  size_t *resp_len, uint8_t *io_buf, size_t io_cap);

#ifdef __cplusplus
}
#endif

#endif

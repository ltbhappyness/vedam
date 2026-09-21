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
#include "vii/types.h"
#include "vti/codec.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    VTA_HTTP_WORKERS = 4,
    VTA_HTTP_IO_MAX = 65536
};

struct vta_http_pool;

struct vta_http_slot {
    struct vta_http_pool *pool;
    int index;
    int fd;
    int kick_r;
    int kick_w;
    int busy;
    int started;
    vplatform_thread_t thread;
    uint8_t out_buf[VTA_CODEC_BUF_SIZE];
    uint8_t http_buf[VTA_HTTP_IO_MAX];
    struct vta_codec_mem codec;
};

struct vta_http_pool {
    int listen_fd;
    int stop;
    int poke_r;
    int poke_w;
    int listen_started;
    vplatform_thread_t listen_thread;
    struct vta_http_slot slot[VTA_HTTP_WORKERS];
};

extern const vta_ops_t vta_grpc_ops;

int vta_grpc_bind_tcp(const char *bind_spec, char *bound, size_t bound_cap);
int vta_http_pool_start(struct vta_http_pool *pool, int listen_fd);
void vta_http_pool_stop(struct vta_http_pool *pool);
void vta_grpc_unbind(int fd, const char *uds_path);

/**
 * @brief Address string the listening descriptor bound, including the kernel port.
 */
const char *vta_grpc_bound(vta_desc_t *desc);

/**
 * @brief Number of TCP connect syscalls performed for a connected descriptor.
 *
 * Used by tests to show keep-alive: two calls share one socket.
 *
 * @param[in] desc Connected remote descriptor
 * @return Connect count, or 0 when @p desc is not a connected client.
 */
unsigned vta_grpc_tcp_connects(vta_desc_t *desc);

/**
 * @brief Send one framed protobuf Message and read the framed reply.
 *
 * @p req already carries the five-byte grpc-web header. The reply body is
 * unframed in place inside @p io_buf; *@p resp points at those bytes.
 *
 * Reads the response using Content-Length and leaves the socket open.
 *
 * @param[in]     fd         Connected TCP socket
 * @param[in]     uuid_str   Hyphenated vertical UUID for the path
 * @param[in]     verb_name  Verb path segment
 * @param[in]     req        Framed request bytes (header plus Message)
 * @param[in]     req_len    Length of @p req
 * @param[out]    resp       Points at unframed Message bytes inside @p io_buf
 * @param[out]    resp_len   Length of the unframed Message
 * @param[in,out] io_buf     Connection-owned HTTP scratch buffer
 * @param[in]     io_cap     Capacity of @p io_buf
 * @return V_STATUS_SUCCESS, or a failure code.
 */
v_status_t vta_grpc_http_exchange(int fd, const char *uuid_str, const char *verb_name,
                                  const uint8_t *req, size_t req_len, const uint8_t **resp,
                                  size_t *resp_len, uint8_t *io_buf, size_t io_cap);

#ifdef __cplusplus
}
#endif

#endif

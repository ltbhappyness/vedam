/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * Connect opens the peer queue write-only and a private reply queue.
 * Each request carries a request id and that reply name. A worker already
 * blocked on the listen queue owns the message, runs the verb, and writes
 * one reply on a reply-send handle opened once and reused.
 */
#include "vti/attrwire.h"
#include "vti/class_uuid.h"
#include "vti/codec.h"
#include "vti/dispatch.h"
#include "vti/mq.h"

#include "meta.h"
#include "platform/platform.h"
#include "util/log.h"
#include "util/trace.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum {
    VTA_MQ_NAME_MAX = 64,
    VTA_MQ_BUF_SIZE = VTA_CODEC_BUF_SIZE,
    VTA_MQ_MAX_ATTR = VTA_CODEC_MAX_ATTR,
    VTA_MQ_WORKERS = 4,
    VTA_MQ_VALUE_MAX = VTA_CODEC_VALUE_MAX,
    VTA_MQ_CALL_MS = 5000,
    VTA_MQ_MAGIC_REQ = 0x564d5131u,
    VTA_MQ_MAGIC_REP = 0x564d5152u
};

struct mq_worker_mem {
    struct vta_codec_mem codec;
    uint8_t rx[VTA_MQ_BUF_SIZE];
    uint8_t enc[VTA_MQ_BUF_SIZE];
};

struct mq_worker_arg {
    struct mq_ctx *ctx;
    int index;
};

struct mq_ctx {
    int stop;
    int listening;
    int nworker;
    int wake_r;
    int wake_w;
    vplatform_thread_t worker[VTA_MQ_WORKERS];
    vplatform_mq_t peer;
    vplatform_mq_t reply;
    vplatform_mq_t mq;
    char name[VTA_MQ_NAME_MAX];
    char reply_name[VTA_MQ_NAME_MAX];
    uint32_t next_id;
    uint8_t enc[VTA_MQ_BUF_SIZE];
    uint8_t dec[VTA_MQ_BUF_SIZE];
    vta_codec_ws_t *codec;
    struct mq_worker_mem *wmem;
    struct mq_worker_arg warg[VTA_MQ_WORKERS];
};

/**
 * @brief Store a 32-bit value in network byte order.
 *
 * @param[out] p Destination bytes
 * @param[in]  v Value to store
 */
static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

/**
 * @brief Load a 32-bit value from network byte order.
 *
 * @param[in] p Source bytes
 * @return Host-order value.
 */
static uint32_t get_u32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/**
 * @brief Return non-zero when @p used plus @p add fits in @p cap.
 *
 * @param[in] used Bytes already used
 * @param[in] add  Bytes to add
 * @param[in] cap  Buffer capacity
 * @return 1 when the add fits, otherwise 0.
 */
static int room_for(size_t used, size_t add, size_t cap)
{
    if (add > cap || used > cap - add) {
        return 0;
    }
    return 1;
}

/**
 * @brief Pack one call request: framing plus a protobuf Message body.
 *
 * Encodes the body straight into @p out after the header.
 *
 * @param[in]  msg        Transport message
 * @param[in]  req_id     Request identifier echoed in the reply
 * @param[in]  reply_name Private reply queue name
 * @param[out] out        Output buffer owned by the connection
 * @param[in]  cap        Capacity of @p out
 * @param[out] out_len    Packed length
 * @param[in,out] ws      Connection-owned codec workspace
 * @return 0 on success, -1 when the message does not fit.
 */
static int pack_request(const vta_msg_t *msg, uint32_t req_id, const char *reply_name, uint8_t *out,
                        size_t cap, uint32_t *out_len, vta_codec_ws_t *ws)
{
    const uint8_t *uuid;
    const vi_msg_t *vim;
    size_t body_len;
    size_t hdr_len;
    size_t name_len;
    v_status_t st;

    if (msg == NULL || msg->msg == NULL || ws == NULL) {
        return -1;
    }
    vim = msg->msg;
    if (vim->key_len > cap || vim->attr_count > VTA_MQ_MAX_ATTR) {
        return -1;
    }
    if (vim->key_len > 0 && vim->key == NULL) {
        return -1;
    }
    if (vim->attr_count > 0 && (vim->attr_list == NULL || vim->status_list == NULL)) {
        return -1;
    }
    name_len = strlen(reply_name);
    if (name_len == 0 || name_len >= VTA_MQ_NAME_MAX) {
        return -1;
    }
    uuid = vta_class_uuid_bytes(msg->class_id);
    if (uuid == NULL) {
        return -1;
    }

    /* magic, id, verb, uuid, origin, name_len, name, body_len */
    hdr_len = 36u + name_len + 4u;
    if (hdr_len > cap) {
        return -1;
    }

    st = vta_codec_encode(msg->class_id, vim, V_STATUS_SUCCESS, out + hdr_len, cap - hdr_len,
                          &body_len, ws);
    if (st != V_STATUS_SUCCESS) {
        return -1;
    }

    put_u32(out + 0, VTA_MQ_MAGIC_REQ);
    put_u32(out + 4, req_id);
    put_u32(out + 8, (uint32_t)msg->verb);
    vplatform_memcpy(out + 12, uuid, VTA_CLASS_UUID_LEN);
    put_u32(out + 28, (uint32_t)msg->origin);
    put_u32(out + 32, (uint32_t)name_len);
    vplatform_memcpy(out + 36, reply_name, name_len);
    put_u32(out + 36 + name_len, (uint32_t)body_len);
    *out_len = (uint32_t)(hdr_len + body_len);
    return 0;
}

/**
 * @brief Unpack one call request into @p msg.
 *
 * @param[in]  in         Input buffer
 * @param[in]  in_len     Length of @p in
 * @param[out] msg        Transport message (msg->msg arrays already prepared)
 * @param[out] reply_name Reply queue name
 * @param[in]  name_cap   Capacity of @p reply_name
 * @param[out] req_id     Request identifier
 * @param[in,out] ws      Connection-owned codec workspace
 * @param[out] key_buf    Server-side key copy buffer
 * @param[in]  key_cap    Capacity of @p key_buf
 * @return 0 on success, -2 for an unknown UUID, -1 on a bad frame.
 */
static int unpack_request(const uint8_t *in, uint32_t in_len, vta_msg_t *msg, char *reply_name,
                          uint32_t name_cap, uint32_t *req_id, vta_codec_ws_t *ws, uint8_t *key_buf,
                          size_t key_cap)
{
    uint8_t uuid[VTA_CLASS_UUID_LEN];
    vi_msg_t *vim;
    size_t used;
    uint32_t name_len;
    uint32_t body_len;
    v_status_t st;
    v_status_t overall;
    int unknown_uuid;

    if (msg == NULL || msg->msg == NULL || ws == NULL) {
        return -1;
    }
    vim = msg->msg;
    if (in_len < 40 || get_u32(in) != VTA_MQ_MAGIC_REQ) {
        return -1;
    }
    used = 4;
    *req_id = get_u32(in + used);
    used += 4;
    msg->verb = (v_verb_t)get_u32(in + used);
    used += 4;
    vplatform_memcpy(uuid, in + used, VTA_CLASS_UUID_LEN);
    used += VTA_CLASS_UUID_LEN;
    st = vta_class_from_uuid(uuid, &msg->class_id, &msg->major, &msg->minor);
    if (st == V_STATUS_NOT_IMPLEMENTED) {
        unknown_uuid = 1;
        msg->class_id = V_CLASS_UNSPECIFIED;
        msg->major = 0;
        msg->minor = 0;
    } else if (st != V_STATUS_SUCCESS) {
        return -1;
    } else {
        unknown_uuid = 0;
    }
    msg->origin = (v_origin_t)get_u32(in + used);
    used += 4;
    if (!room_for(used, 4, in_len)) {
        return -1;
    }
    name_len = get_u32(in + used);
    used += 4;
    if (name_len == 0 || name_len >= name_cap || !room_for(used, name_len, in_len)) {
        return -1;
    }
    vplatform_memcpy(reply_name, in + used, name_len);
    reply_name[name_len] = '\0';
    used += name_len;
    if (!room_for(used, 4, in_len)) {
        return -1;
    }
    body_len = get_u32(in + used);
    used += 4;
    if (!room_for(used, body_len, in_len)) {
        return -1;
    }
    if (unknown_uuid) {
        return -2;
    }

    st = vta_codec_decode(msg->class_id, in + used, body_len, vim, &overall, ws, key_buf, key_cap,
                          0);
    if (st != V_STATUS_SUCCESS) {
        return -1;
    }
    (void)overall;
    return 0;
}

/**
 * @brief Pack one reply: framing plus a protobuf Message body.
 *
 * Encodes the body straight into @p out after the twelve-byte header.
 *
 * @param[in]  req_id  Request identifier
 * @param[in]  st      Overall status
 * @param[in]  msg     Transport message with filled attributes
 * @param[out] out     Output buffer
 * @param[in]  cap     Capacity of @p out
 * @param[out] out_len Packed length
 * @param[in,out] ws   Connection-owned codec workspace
 * @return 0 on success, -1 when the message does not fit.
 */
static int pack_reply(uint32_t req_id, v_status_t st, const vta_msg_t *msg, uint8_t *out,
                      size_t cap, uint32_t *out_len, vta_codec_ws_t *ws)
{
    size_t body_len;
    size_t hdr_len;
    v_status_t rc;

    if (msg == NULL || msg->msg == NULL || ws == NULL) {
        return -1;
    }
    hdr_len = 12u;
    if (hdr_len > cap) {
        return -1;
    }
    rc = vta_codec_encode(msg->class_id, msg->msg, st, out + hdr_len, cap - hdr_len, &body_len, ws);
    if (rc != V_STATUS_SUCCESS) {
        return -1;
    }
    put_u32(out + 0, VTA_MQ_MAGIC_REP);
    put_u32(out + 4, req_id);
    put_u32(out + 8, (uint32_t)body_len);
    *out_len = (uint32_t)(hdr_len + body_len);
    return 0;
}

/**
 * @brief Unpack a reply into the caller's attr_list and status_list in place.
 *
 * Leaves the caller's key pointer unchanged.
 *
 * @param[in]     in     Input buffer
 * @param[in]     in_len Length of @p in
 * @param[out]    req_id Request identifier
 * @param[out]    st     Overall status
 * @param[in,out] msg    Caller's transport message
 * @param[in,out] ws     Connection-owned codec workspace
 * @return 0 on success, -1 on a bad frame.
 */
static int unpack_reply(const uint8_t *in, uint32_t in_len, uint32_t *req_id, v_status_t *st,
                        vta_msg_t *msg, vta_codec_ws_t *ws)
{
    size_t used;
    uint32_t body_len;
    v_status_t rc;

    if (msg == NULL || msg->msg == NULL || ws == NULL) {
        return -1;
    }
    if (in_len < 12 || get_u32(in) != VTA_MQ_MAGIC_REP) {
        return -1;
    }
    used = 4;
    *req_id = get_u32(in + used);
    used += 4;
    body_len = get_u32(in + used);
    used += 4;
    if (!room_for(used, body_len, in_len)) {
        return -1;
    }
    rc = vta_codec_decode(msg->class_id, in + used, body_len, msg->msg, st, ws, NULL, 0, 1);
    if (rc != V_STATUS_SUCCESS) {
        return -1;
    }
    return 0;
}

/**
 * @brief Write one packed reply on a reused send handle.
 *
 * @param[in,out] reply_send Open send handle, or closed with handle -1
 * @param[in,out] open_name  Name currently open on @p reply_send
 * @param[in]     name_cap   Capacity of @p open_name
 * @param[in]     reply_name Desired reply queue name
 * @param[in]     req_id     Request identifier
 * @param[in]     st         Overall status
 * @param[in]     msg        Transport message with attributes
 * @param[out]    enc        Connection-owned encode buffer
 * @param[in]     enc_cap    Capacity of @p enc
 * @param[in,out] ws         Connection-owned codec workspace
 */
static void send_reply(vplatform_mq_t *reply_send, char *open_name, size_t name_cap,
                       const char *reply_name, uint32_t req_id, v_status_t st, const vta_msg_t *msg,
                       uint8_t *enc, size_t enc_cap, vta_codec_ws_t *ws)
{
    uint32_t len;
    int rc;

    rc = pack_reply(req_id, st, msg, enc, enc_cap, &len, ws);
    if (rc != 0) {
        return;
    }

    if (open_name[0] == '\0' || strcmp(open_name, reply_name) != 0) {
        if (reply_send->handle >= 0) {
            vplatform_mq_close_send(reply_send);
            reply_send->handle = (intptr_t)-1;
        }
        rc = vplatform_mq_open_send(reply_send, reply_name);
        if (rc != 0) {
            open_name[0] = '\0';
            return;
        }
        vplatform_strlcpy(open_name, reply_name, name_cap);
    }

    rc = vplatform_mq_send(reply_send, enc, len);
    (void)rc;
}

/**
 * @brief Block on the listen queue, run one verb, and write the reply.
 *
 * @param[in] arg Worker argument with context and index
 * @return Always NULL.
 */
static void *mq_worker(void *arg)
{
    struct mq_worker_arg *wa = (struct mq_worker_arg *)arg;
    struct mq_ctx *ctx = wa->ctx;
    struct mq_worker_mem *wm = &ctx->wmem[wa->index];
    char reply_name[VTA_MQ_NAME_MAX];
    char open_reply[VTA_MQ_NAME_MAX];
    vplatform_mq_t reply_send;

    reply_send.handle = (intptr_t)-1;
    open_reply[0] = '\0';

    while (__atomic_load_n(&ctx->stop, __ATOMIC_ACQUIRE) == 0) {
        struct pollfd pfd[2];
        vta_msg_t req;
        vi_msg_t vim;
        uint32_t req_id;
        v_status_t st;
        intptr_t handle;
        char bit;
        int nfd;
        int pr;
        int n;
        int rc;
        int kind;
        vuint32_t i;

        handle = ctx->mq.handle;
        if (handle < 0 || ctx->wake_r < 0) {
            break;
        }
        nfd = 0;
        pfd[nfd].fd = ctx->wake_r;
        pfd[nfd].events = POLLIN;
        nfd++;
        pfd[nfd].fd = (int)handle;
        pfd[nfd].events = POLLIN;
        nfd++;
        pr = poll(pfd, nfd, -1);
        if (__atomic_load_n(&ctx->stop, __ATOMIC_ACQUIRE) != 0) {
            break;
        }
        if (pr < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if ((pfd[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            break;
        }
        if (pfd[0].revents & POLLIN) {
            while (read(ctx->wake_r, &bit, 1) > 0) {
            }
            continue;
        }
        if ((pfd[1].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            break;
        }
        if ((pfd[1].revents & POLLIN) == 0) {
            continue;
        }

        n = vplatform_mq_receive(&ctx->mq, (char *)wm->rx, sizeof(wm->rx));
        if (n < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            break;
        }

        vplatform_memset(&req, 0, sizeof(req));
        vplatform_memset(&vim, 0, sizeof(vim));
        vplatform_memset(wm->codec.attrs, 0, sizeof(wm->codec.attrs));
        vplatform_memset(wm->codec.statuses, 0, sizeof(wm->codec.statuses));
        vim.attr_count = VTA_MQ_MAX_ATTR;
        vim.attr_list = wm->codec.attrs;
        vim.status_list = wm->codec.statuses;
        for (i = 0; i < VTA_MQ_MAX_ATTR; i++) {
            wm->codec.attrs[i].value.ptr = wm->codec.scratch[i];
            wm->codec.attrs[i].value.text.buf = (char *)wm->codec.scratch[i];
            wm->codec.attrs[i].value.text.len = 0;
        }
        req.msg = &vim;
        rc = unpack_request(wm->rx, (uint32_t)n, &req, reply_name, sizeof(reply_name), &req_id,
                            wm->codec.ws, wm->codec.key, sizeof(wm->codec.key));
        if (rc == -2) {
            vim.attr_count = 0;
            vim.attr_list = NULL;
            vim.status_list = NULL;
            st = V_STATUS_NOT_IMPLEMENTED;
            send_reply(&reply_send, open_reply, sizeof(open_reply), reply_name, req_id, st, &req,
                       wm->enc, sizeof(wm->enc), wm->codec.ws);
            continue;
        }
        if (rc != 0) {
            continue;
        }

        for (i = 0; i < vim.attr_count; i++) {
            kind = vta_attr_kind(req.class_id, wm->codec.attrs[i].id);
            if (kind == VIA_T_PTR) {
                wm->codec.attrs[i].value.ptr = wm->codec.scratch[i];
            } else if (kind == VIA_T_TEXT) {
                wm->codec.attrs[i].value.text.buf = (char *)wm->codec.scratch[i];
                wm->codec.attrs[i].value.text.len = 0;
            }
        }

        st = vta_dispatch(&req);
        send_reply(&reply_send, open_reply, sizeof(open_reply), reply_name, req_id, st, &req,
                   wm->enc, sizeof(wm->enc), wm->codec.ws);
    }

    if (reply_send.handle >= 0) {
        vplatform_mq_close_send(&reply_send);
    }
    return NULL;
}

/**
 * @brief Wake and join every message-queue worker.
 *
 * @param[in,out] ctx Listen context
 */
static void mq_stop_workers(struct mq_ctx *ctx)
{
    char bit;
    int i;

    __atomic_store_n(&ctx->stop, 1, __ATOMIC_RELEASE);
    bit = 1;
    if (ctx->wake_w >= 0) {
        for (i = 0; i < ctx->nworker; i++) {
            (void)write(ctx->wake_w, &bit, 1);
        }
    }
    for (i = 0; i < ctx->nworker; i++) {
        (void)vplatform_thread_join(&ctx->worker[i]);
    }
    ctx->nworker = 0;
    if (ctx->wake_r >= 0) {
        close(ctx->wake_r);
        ctx->wake_r = -1;
    }
    if (ctx->wake_w >= 0) {
        close(ctx->wake_w);
        ctx->wake_w = -1;
    }
    vplatform_mq_close(&ctx->mq, ctx->name);
}

/**
 * @brief Free worker codec workspaces and the worker memory block.
 *
 * @param[in,out] ctx Context that may own worker memory
 */
static void mq_free_workers(struct mq_ctx *ctx)
{
    int i;

    if (ctx->wmem == NULL) {
        return;
    }
    for (i = 0; i < VTA_MQ_WORKERS; i++) {
        vta_codec_ws_destroy(ctx->wmem[i].codec.ws);
        ctx->wmem[i].codec.ws = NULL;
    }
    vplatform_free(ctx->wmem);
    ctx->wmem = NULL;
}

/**
 * @brief Stop workers, close queues, and free the message-queue context.
 *
 * @param[in,out] desc Transport descriptor
 * @return V_STATUS_SUCCESS.
 */
static v_status_t mq_close(vta_desc_t *desc)
{
    struct mq_ctx *ctx = (struct mq_ctx *)desc->ctx;

    if (ctx == NULL) {
        return V_STATUS_SUCCESS;
    }
    if (ctx->listening) {
        mq_stop_workers(ctx);
        mq_free_workers(ctx);
    } else {
        vplatform_mq_close_send(&ctx->peer);
        vplatform_mq_close(&ctx->reply, ctx->reply_name);
        vta_codec_ws_destroy(ctx->codec);
        ctx->codec = NULL;
    }
    vplatform_free(ctx);
    desc->ctx = NULL;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Create the listen queue and start the worker pool.
 *
 * @param[in,out] desc    Transport descriptor
 * @param[in]     address Queue name, or NULL for the per-process default
 * @return V_STATUS_SUCCESS, or a failure code.
 */
static v_status_t mq_listen(vta_desc_t *desc, const char *address)
{
    struct mq_ctx *ctx;
    int pipes[2];
    int rc;
    int i;

    ctx = vplatform_calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    ctx->mq.handle = (intptr_t)-1;
    ctx->peer.handle = (intptr_t)-1;
    ctx->reply.handle = (intptr_t)-1;
    ctx->wake_r = -1;
    ctx->wake_w = -1;
    desc->ctx = ctx;
    vtrace_enter();
    if (address != NULL && address[0] != '\0') {
        vplatform_strlcpy(ctx->name, address, sizeof(ctx->name));
    } else {
        rc = snprintf(ctx->name, sizeof(ctx->name), "/vedam.%ld", vplatform_pid());
        if (rc < 0 || (size_t)rc >= sizeof(ctx->name)) {
            goto out_free;
        }
    }

    ctx->wmem = vplatform_calloc(VTA_MQ_WORKERS, sizeof(*ctx->wmem));
    if (ctx->wmem == NULL) {
        goto out_free;
    }
    for (i = 0; i < VTA_MQ_WORKERS; i++) {
        ctx->wmem[i].codec.ws = vta_codec_ws_create();
        if (ctx->wmem[i].codec.ws == NULL) {
            goto out_wmem;
        }
    }

    rc = pipe(pipes);
    if (rc != 0) {
        goto out_wmem;
    }
    rc = fcntl(pipes[0], F_SETFL, O_NONBLOCK);
    if (rc != 0) {
        close(pipes[0]);
        close(pipes[1]);
        goto out_wmem;
    }
    ctx->wake_r = pipes[0];
    ctx->wake_w = pipes[1];

    rc = vplatform_mq_open_recv(&ctx->mq, ctx->name, 10, VTA_MQ_BUF_SIZE);
    if (rc != 0) {
        vlog_error("mq open failed");
        goto out_wake;
    }
    ctx->listening = 1;
    for (i = 0; i < VTA_MQ_WORKERS; i++) {
        ctx->warg[i].ctx = ctx;
        ctx->warg[i].index = i;
        rc = vplatform_thread_create(&ctx->worker[i], mq_worker, &ctx->warg[i]);
        if (rc != 0) {
            goto out_stop;
        }
        ctx->nworker++;
    }
    vlog_info("mq listen %s", ctx->name);
    vtrace_exit();
    return V_STATUS_SUCCESS;

out_stop:
    mq_stop_workers(ctx);
    mq_free_workers(ctx);
    desc->ctx = NULL;
    vplatform_free(ctx);
    return V_STATUS_FAILURE;
out_wake:
    if (ctx->wake_r >= 0) {
        close(ctx->wake_r);
        ctx->wake_r = -1;
    }
    if (ctx->wake_w >= 0) {
        close(ctx->wake_w);
        ctx->wake_w = -1;
    }
out_wmem:
    mq_free_workers(ctx);
out_free:
    vplatform_free(ctx);
    desc->ctx = NULL;
    return V_STATUS_FAILURE;
}

/**
 * @brief Accept is unused; workers read the listen queue directly.
 *
 * @param[in]  desc   Listen descriptor
 * @param[out] client Unused
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
static v_status_t mq_accept(vta_desc_t *desc, vta_desc_t **client)
{
    (void)desc;
    (void)client;
    return V_STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Open the peer queue and a private reply queue.
 *
 * @param[in,out] desc    Transport descriptor
 * @param[in]     address Peer listen queue name
 * @return V_STATUS_SUCCESS, or a failure code.
 */
static v_status_t mq_connect(vta_desc_t *desc, const char *address)
{
    struct mq_ctx *ctx;
    int rc;

    if (address == NULL || address[0] == '\0') {
        return V_STATUS_INVALID_PARAMETER;
    }
    ctx = vplatform_calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    ctx->peer.handle = (intptr_t)-1;
    ctx->reply.handle = (intptr_t)-1;
    ctx->mq.handle = (intptr_t)-1;
    vplatform_strlcpy(ctx->name, address, sizeof(ctx->name));

    ctx->codec = vta_codec_ws_create();
    if (ctx->codec == NULL) {
        goto out_free;
    }

    rc = vplatform_mq_open_send(&ctx->peer, ctx->name);
    if (rc != 0) {
        vlog_error("mq send open %s errno %d", ctx->name, errno);
        goto out_codec;
    }

    rc = vplatform_mq_open_reply(&ctx->reply, ctx->reply_name, sizeof(ctx->reply_name));
    if (rc != 0) {
        vlog_error("mq reply open errno %d", errno);
        goto out_peer;
    }
    desc->ctx = ctx;
    return V_STATUS_SUCCESS;

out_peer:
    vplatform_mq_close_send(&ctx->peer);
out_codec:
    vta_codec_ws_destroy(ctx->codec);
    ctx->codec = NULL;
out_free:
    vplatform_free(ctx);
    return V_STATUS_FAILURE;
}

/**
 * @brief Send one request and wait for the matching reply.
 *
 * Encodes and decodes through the shared codec into connection buffers.
 * Does not open a message queue.
 *
 * @param[in]     desc Transport descriptor
 * @param[in,out] msg  Call and reply message
 * @return Overall status from the peer, or a local failure code.
 */
static v_status_t mq_call(vta_desc_t *desc, vta_msg_t *msg)
{
    struct mq_ctx *ctx = (struct mq_ctx *)desc->ctx;
    struct pollfd pfd;
    uint32_t len;
    uint32_t req_id;
    uint32_t got_id;
    v_status_t st;
    int rc;
    int pr;
    int rn;

    if (ctx == NULL || ctx->listening || msg == NULL || msg->msg == NULL || ctx->codec == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (msg->msg->attr_count > 0 &&
        (msg->msg->attr_list == NULL || msg->msg->status_list == NULL)) {
        return V_STATUS_INVALID_PARAMETER;
    }
    ctx->next_id++;
    if (ctx->next_id == 0) {
        ctx->next_id = 1;
    }
    req_id = ctx->next_id;

    rc = pack_request(msg, req_id, ctx->reply_name, ctx->enc, sizeof(ctx->enc), &len, ctx->codec);
    if (rc != 0) {
        return V_STATUS_INVALID_PARAMETER;
    }

    rc = vplatform_mq_send(&ctx->peer, ctx->enc, len);
    if (rc != 0) {
        return V_STATUS_FAILURE;
    }

    pfd.fd = (int)ctx->reply.handle;
    pfd.events = POLLIN;
    for (;;) {
        pr = poll(&pfd, 1, VTA_MQ_CALL_MS);
        if (pr <= 0) {
            return V_STATUS_FAILURE;
        }

        rn = vplatform_mq_receive(&ctx->reply, (char *)ctx->dec, sizeof(ctx->dec));
        if (rn < 0) {
            return V_STATUS_FAILURE;
        }
        rc = unpack_reply(ctx->dec, (uint32_t)rn, &got_id, &st, msg, ctx->codec);
        if (rc != 0) {
            return V_STATUS_FAILURE;
        }
        if (got_id != req_id) {
            continue;
        }
        return st;
    }
}

/**
 * @brief Return the bound listen queue name.
 *
 * @param[in] desc Listen descriptor
 * @return Queue name, or NULL when @p desc is not listening.
 */
const char *vta_mq_bound(vta_desc_t *desc)
{
    struct mq_ctx *ctx;

    if (desc == NULL) {
        return NULL;
    }
    ctx = (struct mq_ctx *)desc->ctx;
    if (ctx == NULL || !ctx->listening) {
        return NULL;
    }
    return ctx->name;
}

const vta_ops_t vta_mq_ops = {
    .close = mq_close,
    .listen = mq_listen,
    .accept = mq_accept,
    .connect = mq_connect,
    .call = mq_call,
};

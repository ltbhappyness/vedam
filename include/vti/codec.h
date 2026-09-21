/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VTI_CODEC_H
#define VTI_CODEC_H

#include "vii/types.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    VTA_CODEC_BUF_SIZE = 8192,
    VTA_CODEC_MAX_ATTR = 32,
    VTA_CODEC_VALUE_MAX = 256,
    VTA_CODEC_KEY_MAX = 256
};

/**
 * Opaque protobuf Arena and Message owned by a listen or connect context.
 */
typedef struct vta_codec_ws vta_codec_ws_t;

/**
 * Connection-owned scratch for one encode/decode/HTTP serve path.
 *
 * Allocated once with the listen slot or connect context. Not used on the
 * stack of a per-call path.
 */
struct vta_codec_mem {
    vta_codec_ws_t *ws;
    uint8_t scratch[VTA_CODEC_MAX_ATTR][VTA_CODEC_VALUE_MAX];
    uint8_t key[VTA_CODEC_KEY_MAX];
    v_attr_t attrs[VTA_CODEC_MAX_ATTR];
    v_status_t statuses[VTA_CODEC_MAX_ATTR];
};

/**
 * @brief Allocate a codec workspace (Arena and one Message).
 *
 * @return Workspace pointer, or NULL when allocation fails.
 */
vta_codec_ws_t *vta_codec_ws_create(void);

/**
 * @brief Free a codec workspace.
 *
 * @param[in] ws Workspace from vta_codec_ws_create, or NULL
 */
void vta_codec_ws_destroy(vta_codec_ws_t *ws);

/**
 * @brief Encode @p msg as a protobuf Message into @p buf.
 *
 * Reuses the Message on @p ws. Does not allocate a process-wide Message.
 *
 * @param[in]  class_id Class index used for attribute type kinds
 * @param[in]  msg      Public message to encode
 * @param[in]  overall  Overall status written into Message.status
 * @param[out] buf      Connection-owned output buffer
 * @param[in]  cap      Capacity of @p buf
 * @param[out] out_len  Encoded byte count
 * @param[in,out] ws    Connection-owned codec workspace
 * @return V_STATUS_SUCCESS, or a failure when the message does not fit.
 */
v_status_t vta_codec_encode(v_class_id_t class_id, const vi_msg_t *msg, v_status_t overall,
                            uint8_t *buf, size_t cap, size_t *out_len, vta_codec_ws_t *ws);

/**
 * @brief Decode a protobuf Message into @p msg.
 *
 * Scalar values land in attr_list[i].value. Text and pointer bytes land in the
 * buffers the caller already set on those attributes. @p attr_count is the
 * capacity of the attribute arrays on entry and the decoded count on success.
 *
 * When @p preserve_key is non-zero, @p msg->key and @p msg->key_len are left
 * unchanged (client reply path). Otherwise key bytes are copied into @p key_buf
 * and @p msg->key points at that copy for the rest of the call (server path).
 *
 * @param[in]     class_id     Class index used for attribute type kinds
 * @param[in]     buf          Encoded Message bytes
 * @param[in]     len          Length of @p buf
 * @param[in,out] msg          Public message to fill
 * @param[out]    overall      Overall status from Message.status
 * @param[in,out] ws           Connection-owned codec workspace
 * @param[out]    key_buf      Server-side key copy buffer, or NULL when unused
 * @param[in]     key_cap      Capacity of @p key_buf
 * @param[in]     preserve_key Non-zero to leave the caller's key pointer alone
 * @return V_STATUS_SUCCESS, or a failure when the frame is bad.
 */
v_status_t vta_codec_decode(v_class_id_t class_id, const uint8_t *buf, size_t len, vi_msg_t *msg,
                            v_status_t *overall, vta_codec_ws_t *ws, uint8_t *key_buf, size_t key_cap,
                            int preserve_key);

/**
 * @brief Serve one HTTP verb using connection-owned @p mem and @p out_buf.
 *
 * @param[in]  class_name  Hyphenated vertical UUID from the path
 * @param[in]  verb_name   Verb path segment
 * @param[in]  body        Request body bytes (grpc frame already stripped)
 * @param[in]  body_len    Length of @p body
 * @param[out] out_buf     Connection-owned response buffer
 * @param[in]  out_cap     Capacity of @p out_buf
 * @param[out] out_len     Response byte count including the five-byte prefix
 * @param[out] grpc_status Static digit string for the trailer
 * @param[in,out] mem      Connection-owned codec memory
 * @return 0 when a response was produced, -1 when the body cannot be read.
 */
int vta_codec_http(const char *class_name, const char *verb_name, const uint8_t *body,
                   size_t body_len, uint8_t *out_buf, size_t out_cap, size_t *out_len,
                   const char **grpc_status, struct vta_codec_mem *mem);

/**
 * @brief Map a library status to a grpc-web digit string.
 *
 * @param[in] st Library status
 * @return Static digit string for the HTTP trailer.
 */
const char *vta_codec_grpc_digits(v_status_t st);

/**
 * @brief Write the five-byte grpc-web frame header at the front of @p buf.
 *
 * The encoded Message must already occupy @p buf[5 .. 5 + payload_len).
 *
 * @param[in,out] buf         Buffer with payload at offset 5
 * @param[in]     buf_cap     Capacity of @p buf
 * @param[in]     payload_len Encoded Message length after the header
 * @param[out]    out_len     Total framed length (5 + payload_len)
 * @return V_STATUS_SUCCESS, or V_STATUS_BUFFER_OVERFLOW when @p buf is too small.
 */
v_status_t vta_codec_grpc_frame(uint8_t *buf, size_t buf_cap, size_t payload_len, size_t *out_len);

/**
 * @brief Strip a five-byte grpc-web frame header when present.
 *
 * @param[in,out] body     Points at the body start
 * @param[in,out] body_len Body length
 */
void vta_codec_grpc_unframe(const uint8_t **body, size_t *body_len);

/**
 * @brief Verb path segment for @p verb, or NULL when unknown.
 *
 * @param[in] verb Verb identifier
 * @return Static name string, or NULL.
 */
const char *vta_codec_verb_name(v_verb_t verb);

/**
 * @brief Parse a verb path segment into @p verb.
 *
 * @param[in]  name Verb path segment
 * @param[out] verb Receives the verb id
 * @return V_STATUS_SUCCESS, or V_STATUS_NOT_IMPLEMENTED when unknown.
 */
v_status_t vta_codec_verb_parse(const char *name, v_verb_t *verb);

#ifdef __cplusplus
}
#endif

#endif

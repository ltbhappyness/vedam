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
    VTI_CODEC_BUF_SIZE = 8192,
    VTI_CODEC_MAX_ATTR = 32,
    VTI_CODEC_VALUE_MAX = 256,
    VTI_CODEC_KEY_MAX = 256
};

/**
 * Opaque protobuf Arena and Message owned by a listen or connect context.
 */
typedef struct vti_codec_ws vti_codec_ws_t;

/**
 * Connection-owned scratch for one encode/decode/HTTP serve path.
 *
 * Allocated once with the listen slot or connect context. Not used on the
 * stack of a per-call path. Attribute i keeps its text, list items, or struct
 * in scratch[i]; that slot is the room a server gives the value.
 */
struct vti_codec_mem {
    vti_codec_ws_t *ws;
    uint8_t scratch[VTI_CODEC_MAX_ATTR][VTI_CODEC_VALUE_MAX];
    uint8_t key[VTI_CODEC_KEY_MAX];
    v_attr_t attrs[VTI_CODEC_MAX_ATTR];
    v_status_t statuses[VTI_CODEC_MAX_ATTR];
};

/**
 * @brief Allocate a codec workspace (Arena and one Message).
 *
 * @return Workspace pointer, or NULL when allocation fails.
 */
vti_codec_ws_t *vti_codec_ws_create(void);

/**
 * @brief Free a codec workspace.
 *
 * @param[in] ws Workspace from vti_codec_ws_create, or NULL
 */
void vti_codec_ws_destroy(vti_codec_ws_t *ws);

/**
 * @brief Encode the message of @p call as a protobuf Message into @p buf.
 *
 * Reuses the Message on @p ws. Does not allocate a process-wide Message.
 * A value travels with the data: a request carries values only when the verb
 * writes them; a reply carries the value of each attribute that succeeded,
 * only when the verb does not write.
 *
 * @param[in]  call     Call with class, verb, and the public message to encode
 * @param[in]  overall  Overall status written into Message.status
 * @param[in]  reply    Non-zero when encoding a reply, zero for a request
 * @param[out] buf      Connection-owned output buffer
 * @param[in]  cap      Capacity of @p buf
 * @param[out] out_len  Encoded byte count
 * @param[in,out] ws    Connection-owned codec workspace
 * @return V_STATUS_SUCCESS, or a failure when the message does not fit.
 */
v_status_t vti_codec_encode(const vii_msg_t *call, v_status_t overall, int reply, uint8_t *buf,
                            size_t cap, size_t *out_len, vti_codec_ws_t *ws);

/**
 * @brief Decode a protobuf Message into the message of @p call.
 *
 * The class and verb of @p call are set on entry. @p attr_count is the room in
 * the attribute arrays on entry and the decoded count on success.
 *
 * With @p mem (server), the key is copied into @p mem and each attribute is
 * pointed at its value slot. On a verb that writes values the decoded value is
 * kept when it fits the slot; on any other verb the slot's room goes into the
 * text len or list count. A value that does not fit gives that attribute
 * V_STATUS_BUFFER_OVERFLOW, and @p overall is the first such failure.
 *
 * Without @p mem (client), the caller's key is left alone. On a verb that does
 * not write values, each value lands in the caller's buffers; a text len or
 * list count is the room on entry. A value that does not fit is not copied, its
 * len or count holds the needed size, and that attribute gets
 * V_STATUS_BUFFER_OVERFLOW. @p overall is the peer's overall status, or the
 * first such failure when the peer succeeded.
 *
 * @param[in]     buf     Encoded Message bytes
 * @param[in]     len     Length of @p buf
 * @param[in,out] call    Call with class and verb set, and the public message to fill
 * @param[out]    overall Overall status
 * @param[in,out] ws      Connection-owned codec workspace
 * @param[in,out] mem     Server codec memory prepared by vti_codec_rx_prepare, or NULL on a client
 * @return V_STATUS_SUCCESS, or a failure when the frame is bad.
 */
v_status_t vti_codec_decode(const uint8_t *buf, size_t len, vii_msg_t *call, v_status_t *overall,
                            vti_codec_ws_t *ws, struct vti_codec_mem *mem);

/**
 * @brief Point @p msg at the attribute and status slots of @p mem and clear them.
 *
 * A server calls this before vti_codec_decode on every request.
 *
 * @param[in,out] mem Server codec memory
 * @param[out]    msg Server message to fill by vti_codec_decode
 */
void vti_codec_rx_prepare(struct vti_codec_mem *mem, vi_msg_t *msg);

/**
 * @brief Serve one HTTP verb using connection-owned @p mem and @p out_buf.
 *
 * An unknown service or method produces no body and grpc status 12. A request
 * value that does not fit its slot is answered with V_STATUS_BUFFER_OVERFLOW for
 * that attribute, and the verb does not run.
 *
 * @param[in]  service     Service name from the path, such as vedam.system.v1.SystemService
 * @param[in]  method      Method name from the path, such as Get
 * @param[in]  body        Request body bytes (grpc frame already stripped)
 * @param[in]  body_len    Length of @p body
 * @param[out] out_buf     Connection-owned response buffer
 * @param[in]  out_cap     Capacity of @p out_buf
 * @param[out] out_len     Response byte count including the five-byte prefix
 * @param[out] grpc_status Static digit string for the trailer
 * @param[in,out] mem      Connection-owned codec memory
 * @return 0 when a response was produced, -1 when the body cannot be read.
 */
int vti_codec_http(const char *service, const char *method, const uint8_t *body, size_t body_len,
                   uint8_t *out_buf, size_t out_cap, size_t *out_len, const char **grpc_status,
                   struct vti_codec_mem *mem);

/**
 * @brief Map a library status to a grpc-web digit string.
 *
 * @param[in] st Library status
 * @return Static digit string for the HTTP trailer.
 */
const char *vti_codec_grpc_digits(v_status_t st);

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
v_status_t vti_codec_grpc_frame(uint8_t *buf, size_t buf_cap, size_t payload_len, size_t *out_len);

/**
 * @brief Strip a five-byte grpc-web frame header when present.
 *
 * @param[in,out] body     Points at the body start
 * @param[in,out] body_len Body length
 */
void vti_codec_grpc_unframe(const uint8_t **body, size_t *body_len);

/**
 * @brief Method name for @p verb, or NULL when unknown.
 *
 * @param[in] verb Verb identifier
 * @return Static name string, or NULL.
 */
const char *vti_codec_verb_name(v_verb_t verb);

/**
 * @brief Parse a method name into @p verb.
 *
 * @param[in]  name Method name
 * @param[out] verb Receives the verb id
 * @return V_STATUS_SUCCESS, or V_STATUS_NOT_IMPLEMENTED when unknown.
 */
v_status_t vti_codec_verb_parse(const char *name, v_verb_t *verb);

#ifdef __cplusplus
}

namespace google
{
namespace protobuf
{
class Any;
} // namespace protobuf
} // namespace google

/**
 * @brief Pack and unpack one class struct as a protobuf message inside an Any.
 *
 * put builds the message on the arena that owns @p dst. take writes every
 * member of the C struct, and zero for a member that does not travel.
 */
struct vti_struct_codec {
    v_status_t (*put)(const void *c, google::protobuf::Any *dst);
    v_status_t (*take)(const google::protobuf::Any &src, void *c);
};

/**
 * @brief Struct codecs indexed by vii_attr_meta.struct_index.
 */
extern const struct vti_struct_codec vti_struct_codecs[];
#endif /* __cplusplus */

#endif /* VTI_CODEC_H */

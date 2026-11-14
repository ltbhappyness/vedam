/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "vti/codec.h"

#include <arpa/inet.h>

#include <cstring>
#include <memory>
#include <new>

#include <google/protobuf/arena.h>

#include "core/node.h"
#include "platform/memory.h"
#include "util/attr.h"
#include "vedam/v1/types.pb.h"
#include "vii/dispatch.h"
#include "vii/meta.h"
#include "vti/class_uuid.h"

struct vti_codec_ws {
    google::protobuf::Arena arena;
    vedam::v1::Message *wire;

    vti_codec_ws() : arena(), wire(nullptr)
    {
        wire = google::protobuf::Arena::CreateMessage<vedam::v1::Message>(&arena);
    }

    /**
     * @brief Reset the Arena and recreate the reusable Message.
     */
    void recycle(void)
    {
        arena.Reset();
        wire = google::protobuf::Arena::CreateMessage<vedam::v1::Message>(&arena);
    }
};

/* vattr_write_watch stores one bit per slot, for 32 slots, and does not include this header. */
static_assert(VTI_CODEC_MAX_ATTR <= 32, "write watch bits cover the server attribute array");

namespace
{

/**
 * @brief Map a library status to a protobuf StatusCode without collapsing errors.
 *
 * @param[in] st Library status
 * @return Matching StatusCode, including indexed-attribute families.
 */
static vedam::v1::StatusCode to_proto_code(v_status_t st)
{
    if (st == V_STATUS_SUCCESS) {
        return vedam::v1::STATUS_OK;
    }
    if (st == V_STATUS_FAILURE) {
        return vedam::v1::STATUS_FAILURE;
    }
    if (st == V_STATUS_NOT_SUPPORTED) {
        return vedam::v1::STATUS_NOT_SUPPORTED;
    }
    if (st == V_STATUS_NO_MEMORY) {
        return vedam::v1::STATUS_NO_MEMORY;
    }
    if (st == V_STATUS_INVALID_PARAMETER) {
        return vedam::v1::STATUS_INVALID_PARAMETER;
    }
    if (st == V_STATUS_ITEM_ALREADY_EXISTS) {
        return vedam::v1::STATUS_ITEM_ALREADY_EXISTS;
    }
    if (st == V_STATUS_ITEM_NOT_FOUND) {
        return vedam::v1::STATUS_ITEM_NOT_FOUND;
    }
    if (st == V_STATUS_BUFFER_OVERFLOW) {
        return vedam::v1::STATUS_BUFFER_OVERFLOW;
    }
    if (st == V_STATUS_MANDATORY_ATTR_MISSING) {
        return vedam::v1::STATUS_MANDATORY_ATTR_MISSING;
    }
    if (st == V_STATUS_UNINITIALIZED) {
        return vedam::v1::STATUS_UNINITIALIZED;
    }
    if (st == V_STATUS_NOT_IMPLEMENTED) {
        return vedam::v1::STATUS_NOT_IMPLEMENTED;
    }
    if (st == V_STATUS_FAILED_PRECONDITION) {
        return vedam::v1::STATUS_FAILED_PRECONDITION;
    }
    if (st == V_STATUS_CAPABILITY_DENIED) {
        return vedam::v1::STATUS_CAPABILITY_DENIED;
    }
    if (st == V_STATUS_OFFLINE) {
        return vedam::v1::STATUS_OFFLINE;
    }
    if (st == V_STATUS_THERMAL_SHED) {
        return vedam::v1::STATUS_THERMAL_SHED;
    }
    if (st == V_STATUS_OBJECT_IN_USE) {
        return vedam::v1::STATUS_OBJECT_IN_USE;
    }
    if (st <= V_STATUS_INVALID_ATTR_VALUE_0 && st > V_STATUS_INVALID_ATTR_VALUE_0 - 1000) {
        return vedam::v1::STATUS_INVALID_ATTR_VALUE;
    }
    if (st <= V_STATUS_ATTR_NOT_IMPLEMENTED_0 && st > V_STATUS_ATTR_NOT_IMPLEMENTED_0 - 1000) {
        return vedam::v1::STATUS_ATTR_NOT_IMPLEMENTED;
    }
    if (st <= V_STATUS_UNKNOWN_ATTRIBUTE_0 && st > V_STATUS_UNKNOWN_ATTRIBUTE_0 - 1000) {
        return vedam::v1::STATUS_UNKNOWN_ATTRIBUTE;
    }
    if (st <= V_STATUS_ATTR_NOT_SUPPORTED_0 && st > V_STATUS_ATTR_NOT_SUPPORTED_0 - 1000) {
        return vedam::v1::STATUS_ATTR_NOT_SUPPORTED;
    }
    if (st <= V_STATUS_INVALID_ATTR_ID_0 && st > V_STATUS_INVALID_ATTR_ID_0 - 1000) {
        return vedam::v1::STATUS_INVALID_ATTR_ID;
    }
    return vedam::v1::STATUS_FAILURE;
}

/**
 * @brief Map a protobuf StatusCode back to a library status.
 *
 * @param[in] code       Wire status code
 * @param[in] attr_index Zero-based attribute index for indexed families
 * @return Matching v_status_t.
 */
static v_status_t from_proto_code(vedam::v1::StatusCode code, int attr_index)
{
    switch (code) {
    case vedam::v1::STATUS_OK:
        return V_STATUS_SUCCESS;
    case vedam::v1::STATUS_FAILURE:
        return V_STATUS_FAILURE;
    case vedam::v1::STATUS_NOT_SUPPORTED:
        return V_STATUS_NOT_SUPPORTED;
    case vedam::v1::STATUS_NO_MEMORY:
        return V_STATUS_NO_MEMORY;
    case vedam::v1::STATUS_INVALID_PARAMETER:
        return V_STATUS_INVALID_PARAMETER;
    case vedam::v1::STATUS_ITEM_ALREADY_EXISTS:
        return V_STATUS_ITEM_ALREADY_EXISTS;
    case vedam::v1::STATUS_ITEM_NOT_FOUND:
        return V_STATUS_ITEM_NOT_FOUND;
    case vedam::v1::STATUS_BUFFER_OVERFLOW:
        return V_STATUS_BUFFER_OVERFLOW;
    case vedam::v1::STATUS_MANDATORY_ATTR_MISSING:
        return V_STATUS_MANDATORY_ATTR_MISSING;
    case vedam::v1::STATUS_UNINITIALIZED:
        return V_STATUS_UNINITIALIZED;
    case vedam::v1::STATUS_NOT_IMPLEMENTED:
        return V_STATUS_NOT_IMPLEMENTED;
    case vedam::v1::STATUS_FAILED_PRECONDITION:
        return V_STATUS_FAILED_PRECONDITION;
    case vedam::v1::STATUS_CAPABILITY_DENIED:
        return V_STATUS_CAPABILITY_DENIED;
    case vedam::v1::STATUS_OFFLINE:
        return V_STATUS_OFFLINE;
    case vedam::v1::STATUS_THERMAL_SHED:
        return V_STATUS_THERMAL_SHED;
    case vedam::v1::STATUS_OBJECT_IN_USE:
        return V_STATUS_OBJECT_IN_USE;
    case vedam::v1::STATUS_INVALID_ATTR_VALUE:
        return (v_status_t)(V_STATUS_INVALID_ATTR_VALUE_0 - attr_index);
    case vedam::v1::STATUS_ATTR_NOT_IMPLEMENTED:
        return (v_status_t)(V_STATUS_ATTR_NOT_IMPLEMENTED_0 - attr_index);
    case vedam::v1::STATUS_UNKNOWN_ATTRIBUTE:
        return (v_status_t)(V_STATUS_UNKNOWN_ATTRIBUTE_0 - attr_index);
    case vedam::v1::STATUS_ATTR_NOT_SUPPORTED:
        return (v_status_t)(V_STATUS_ATTR_NOT_SUPPORTED_0 - attr_index);
    case vedam::v1::STATUS_INVALID_ATTR_ID:
        return (v_status_t)(V_STATUS_INVALID_ATTR_ID_0 - attr_index);
    default:
        return V_STATUS_FAILURE;
    }
}

/**
 * @brief Return the row for @p id in @p class_id, or NULL when the class or the id is unknown.
 *
 * @param[in] class_id Class index
 * @param[in] id       Attribute id
 * @return The attribute row, or NULL.
 */
static const struct vii_attr_meta *attr_meta(v_class_id_t class_id, v_attr_id_t id)
{
    vedam_t *node = vedam_node();

    if (node == NULL || (unsigned)class_id > (unsigned)V_CLASS_ID_MAX) {
        return NULL;
    }
    return vii_attr_lookup(&node->classes[class_id], id);
}

/**
 * @brief Return non-zero when the caller sends values on @p verb.
 *
 * A request carries values only on such a verb, and its reply carries none.
 *
 * @param[in] verb Verb from the frame, possibly out of range
 * @return Non-zero when @p verb writes values.
 */
static int writes_values(v_verb_t verb)
{
    return (unsigned)verb < (unsigned)V_VERB_COUNT && vii_verb_rules[verb].writes_values;
}

/**
 * @brief Store one value of a scalar or struct kind into a protobuf AttrValue.
 *
 * A plain attribute and a list item use this same step.
 *
 * @param[in]  kind VII_T_* kind of the value at @p p
 * @param[in]  m    Attribute row, for the struct codec
 * @param[in]  p    One value or one list item
 * @param[out] dst  Protobuf value
 * @return 0 on success, -1 when the value cannot be encoded.
 */
static int put_item(vuint8_t kind, const struct vii_attr_meta *m, const void *p,
                    vedam::v1::AttrValue *dst)
{
    const v_version_t *version;

    switch (kind) {
    case VII_T_BOOL:
        dst->set_booldata(*static_cast<const vbool_t *>(p) != 0);
        return 0;
    case VII_T_S32:
        dst->set_s32(*static_cast<const vint32_t *>(p));
        return 0;
    case VII_T_U32:
        dst->set_u32(*static_cast<const vuint32_t *>(p));
        return 0;
    case VII_T_U64:
        dst->set_u64(*static_cast<const vuint64_t *>(p));
        return 0;
    case VII_T_VERSION:
        version = static_cast<const v_version_t *>(p);
        dst->mutable_version()->set_major(version->major);
        dst->mutable_version()->set_minor(version->minor);
        return 0;
    case VII_T_STRUCT:
        return vti_struct_codecs[m->struct_index].put(p, dst->mutable_message()) == V_STATUS_SUCCESS
                   ? 0
                   : -1;
    default:
        return -1;
    }
}

/**
 * @brief Load one value of a scalar or struct kind from a protobuf AttrValue.
 *
 * A plain attribute and a list item use this same step. A struct with no
 * message is written zero.
 *
 * @param[in]  kind VII_T_* kind of the value at @p p
 * @param[in]  m    Attribute row, for the struct codec and size
 * @param[in]  src  Protobuf value
 * @param[out] p    One value or one list item
 * @return 0 on success, -1 when the value cannot be decoded.
 */
static int take_item(vuint8_t kind, const struct vii_attr_meta *m, const vedam::v1::AttrValue &src,
                     void *p)
{
    v_version_t *version;

    switch (kind) {
    case VII_T_BOOL:
        *static_cast<vbool_t *>(p) = src.booldata() ? 1 : 0;
        return 0;
    case VII_T_S32:
        *static_cast<vint32_t *>(p) = src.s32();
        return 0;
    case VII_T_U32:
        *static_cast<vuint32_t *>(p) = src.u32();
        return 0;
    case VII_T_U64:
        *static_cast<vuint64_t *>(p) = src.u64();
        return 0;
    case VII_T_VERSION:
        version = static_cast<v_version_t *>(p);
        version->major = src.version().major();
        version->minor = src.version().minor();
        return 0;
    case VII_T_STRUCT:
        if (!src.has_message()) {
            vplatform_memset(p, 0, m->size);
            return 0;
        }
        return vti_struct_codecs[m->struct_index].take(src.message(), p) == V_STATUS_SUCCESS ? 0
                                                                                             : -1;
    default:
        return -1;
    }
}

/**
 * @brief Store the items of a list attribute into a protobuf ListValue.
 *
 * @param[in]  m    Attribute row with kind VII_T_LIST
 * @param[in]  src  Caller list
 * @param[out] dst  Protobuf list
 * @return 0 on success, -1 when the list is longer than max_count or an item fails.
 */
static int put_list(const struct vii_attr_meta *m, const v_list_t *src, vedam::v1::ListValue *dst)
{
    const vuint8_t *item = static_cast<const vuint8_t *>(src->items);
    vuint32_t i;

    if (src->count > m->max_count || (src->count > 0 && item == NULL)) {
        return -1;
    }
    for (i = 0; i < src->count; i++) {
        if (put_item(m->elem_kind, m, item + (size_t)i * m->elem_size, dst->add_items()) != 0) {
            return -1;
        }
    }
    return 0;
}

/**
 * @brief Copy text into the receiver's buffer when it fits the room in @p dst->len.
 *
 * @param[in]     src Protobuf value
 * @param[in,out] dst Text whose len is the room on entry and the text length on return
 * @return V_STATUS_SUCCESS, V_STATUS_BUFFER_OVERFLOW, or V_STATUS_INVALID_PARAMETER.
 */
static v_status_t take_text(const vedam::v1::AttrValue &src, v_text_t *dst)
{
    vuint32_t room = dst->len;
    size_t len = src.text().size();

    if (len > UINT32_MAX) {
        return V_STATUS_INVALID_PARAMETER;
    }
    dst->len = static_cast<vuint32_t>(len);
    if (len > room) {
        return V_STATUS_BUFFER_OVERFLOW;
    }
    if (len == 0) {
        return V_STATUS_SUCCESS;
    }
    if (dst->buf == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    vplatform_memcpy(dst->buf, src.text().data(), len);
    return V_STATUS_SUCCESS;
}

/**
 * @brief Copy list items into the receiver's array when they fit the room in @p dst->count.
 *
 * @param[in]     m   Attribute row with kind VII_T_LIST
 * @param[in]     src Protobuf value
 * @param[in,out] dst List whose count is the room on entry and the item count on return
 * @return V_STATUS_SUCCESS, V_STATUS_BUFFER_OVERFLOW, or V_STATUS_INVALID_PARAMETER.
 */
static v_status_t take_list(const struct vii_attr_meta *m, const vedam::v1::AttrValue &src,
                            v_list_t *dst)
{
    vuint8_t *item = static_cast<vuint8_t *>(dst->items);
    vuint32_t room = dst->count;
    int n = src.list().items_size();
    int i;

    dst->count = static_cast<vuint32_t>(n);
    if ((vuint32_t)n > room) {
        return V_STATUS_BUFFER_OVERFLOW;
    }
    if (n > 0 && item == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    for (i = 0; i < n; i++) {
        if (take_item(m->elem_kind, m, src.list().items(i), item + (size_t)i * m->elem_size) != 0) {
            return V_STATUS_INVALID_PARAMETER;
        }
    }
    return V_STATUS_SUCCESS;
}

/**
 * @brief Store one attribute into a protobuf Attr, with its value when @p send is set.
 *
 * An id the class does not define crosses with no value so the node can reject it.
 * A reply never reads past one slot: text longer than the slot, or a list that
 * does not fit it, sends no bytes. A value that fills the slot is sent whole.
 * A request still sends the caller's length, so a peer can refuse an oversize value.
 *
 * @param[in]  m     Attribute row, or NULL when the class does not define the id
 * @param[in]  src   Source attribute
 * @param[in]  send  Non-zero when the value travels in this direction
 * @param[in]  reply Non-zero when encoding a reply, whose buffer is one slot
 * @param[out] dst   Protobuf attribute
 * @return 0 on success, -1 when the value cannot be encoded.
 */
static int put_attr_value(const struct vii_attr_meta *m, const v_attr_t *src, int send, int reply,
                          vedam::v1::Attr *dst)
{
    vedam::v1::AttrValue *value;

    dst->set_id(src->id);
    if (m == NULL || !send) {
        return 0;
    }
    value = dst->mutable_value();
    switch (m->kind) {
    case VII_T_TEXT:
        if (reply && src->value.text.len > VTI_CODEC_VALUE_MAX) {
            return 0;
        }
        if (src->value.text.buf != NULL && src->value.text.len > 0) {
            value->set_text(src->value.text.buf, src->value.text.len);
        } else {
            value->set_text("");
        }
        return 0;
    case VII_T_LIST:
        if (m->elem_size == 0 ||
            (reply && src->value.list.count > VTI_CODEC_VALUE_MAX / m->elem_size)) {
            return 0;
        }
        return put_list(m, &src->value.list, value->mutable_list());
    case VII_T_STRUCT:
        return src->value.ptr == NULL ? 0 : put_item(VII_T_STRUCT, m, src->value.ptr, value);
    default:
        return put_item(m->kind, m, &src->value, value);
    }
}

/**
 * @brief Load one attribute value from a protobuf AttrValue into the receiver's memory.
 *
 * Text and list items are copied only when they fit the room the receiver set.
 *
 * @param[in]     m   Attribute row
 * @param[in]     src Protobuf value
 * @param[in,out] dst Destination attribute (text, list, and struct buffers already set)
 * @return V_STATUS_SUCCESS, V_STATUS_BUFFER_OVERFLOW for this attribute, or
 *         V_STATUS_INVALID_PARAMETER when the value cannot be copied.
 */
static v_status_t take_attr_value(const struct vii_attr_meta *m, const vedam::v1::AttrValue &src,
                                  v_attr_t *dst)
{
    switch (m->kind) {
    case VII_T_TEXT:
        return take_text(src, &dst->value.text);
    case VII_T_LIST:
        return take_list(m, src, &dst->value.list);
    case VII_T_STRUCT:
        if (dst->value.ptr == NULL) {
            return V_STATUS_INVALID_PARAMETER;
        }
        return take_item(VII_T_STRUCT, m, src, dst->value.ptr) == 0 ? V_STATUS_SUCCESS
                                                                    : V_STATUS_INVALID_PARAMETER;
    default:
        return take_item(m->kind, m, src, &dst->value) == 0 ? V_STATUS_SUCCESS
                                                            : V_STATUS_INVALID_PARAMETER;
    }
}

/**
 * @brief Point a server attribute at its value slot and give it the slot's room.
 *
 * Text gets the slot bytes, a list the items that fit, a struct the slot as its
 * struct. When the verb does not write values the slot is cleared, so a body
 * that writes nothing returns no bytes left from an earlier call.
 *
 * @param[in]     m      Attribute row
 * @param[in]     writes Non-zero when the verb writes values
 * @param[in,out] attr   Server attribute
 * @param[out]    slot   Value slot of VTI_CODEC_VALUE_MAX bytes
 */
static void slot_ready(const struct vii_attr_meta *m, int writes, v_attr_t *attr, uint8_t *slot)
{
    switch (m->kind) {
    case VII_T_TEXT:
        attr->value.text.buf = reinterpret_cast<char *>(slot);
        attr->value.text.len = VTI_CODEC_VALUE_MAX;
        break;
    case VII_T_LIST:
        attr->value.list.items = slot;
        attr->value.list.count = m->elem_size == 0 ? 0 : VTI_CODEC_VALUE_MAX / m->elem_size;
        break;
    case VII_T_STRUCT:
        attr->value.ptr = slot;
        break;
    default:
        return;
    }
    if (!writes) {
        vplatform_memset(slot, 0, VTI_CODEC_VALUE_MAX);
    }
}

/**
 * @brief Fill a protobuf Message from @p call and @p overall.
 *
 * A request carries values only when its verb writes them. A reply carries the
 * value of each attribute that succeeded, only when its verb does not write.
 * On that reply a text or list the body did not pass through the writers is
 * sent as length 0. The caller's own lengths are left alone: a request still
 * needs them as the room for the reply.
 *
 * @param[in]  call    Call with class, verb, and the public message
 * @param[in]  overall Overall status
 * @param[in]  reply   Non-zero when encoding a reply
 * @param[out] wire    Protobuf message
 * @return V_STATUS_SUCCESS, or V_STATUS_INVALID_PARAMETER on a bad attribute.
 */
static v_status_t fill_message(const vii_msg_t *call, v_status_t overall, int reply,
                               vedam::v1::Message *wire)
{
    const vi_msg_t *msg = call->msg;
    int writes = writes_values(call->verb);
    vuint32_t i;

    wire->Clear();
    if (msg->key != NULL && msg->key_len > 0) {
        wire->set_key(msg->key, msg->key_len);
    }
    wire->mutable_status()->set_code(to_proto_code(overall));
    wire->mutable_status()->set_vedam_status(static_cast<int32_t>(overall));
    for (i = 0; i < msg->attr_count; i++) {
        vedam::v1::Attr *dst = wire->add_attrs();
        const v_attr_t *src = &msg->attr_list[i];
        const struct vii_attr_meta *meta = attr_meta(call->class_id, src->id);
        v_attr_t cleared;
        v_status_t st = reply ? msg->status_list[i] : V_STATUS_SUCCESS;
        int send = reply ? !writes && st == V_STATUS_SUCCESS : writes;

        if (reply && !writes && meta != NULL && !vattr_was_written(src) &&
            (meta->kind == VII_T_TEXT || meta->kind == VII_T_LIST)) {
            cleared = *src;
            if (meta->kind == VII_T_TEXT) {
                cleared.value.text.len = 0;
            } else {
                cleared.value.list.count = 0;
            }
            src = &cleared;
        }
        if (put_attr_value(meta, src, send, reply, dst) != 0) {
            return V_STATUS_INVALID_PARAMETER;
        }
        dst->mutable_status()->set_code(to_proto_code(st));
        dst->mutable_status()->set_vedam_status(static_cast<int32_t>(st));
    }
    return V_STATUS_SUCCESS;
}

/**
 * @brief Return the status a peer wrote for one attribute.
 *
 * @param[in] src   Protobuf attribute
 * @param[in] index Zero-based attribute index
 * @return The peer's status for that attribute.
 */
static v_status_t peer_status(const vedam::v1::Attr &src, int index)
{
    v_status_t st = (v_status_t)src.status().vedam_status();

    if (st == V_STATUS_SUCCESS && src.status().code() != vedam::v1::STATUS_OK &&
        src.status().code() != vedam::v1::STATUS_CODE_UNSPECIFIED) {
        st = from_proto_code(src.status().code(), index);
    }
    return st;
}

/**
 * @brief Copy the key of @p wire into the server key buffer of @p mem.
 *
 * @param[in]     wire Protobuf message
 * @param[in,out] msg  Server message whose key points at the copy
 * @param[in,out] mem  Server codec memory
 * @return V_STATUS_SUCCESS, or V_STATUS_BUFFER_OVERFLOW when the key is too long.
 */
static v_status_t take_key(const vedam::v1::Message &wire, vi_msg_t *msg, struct vti_codec_mem *mem)
{
    size_t len = wire.key().size();

    if (len == 0) {
        msg->key = NULL;
        msg->key_len = 0;
        return V_STATUS_SUCCESS;
    }
    if (len > sizeof(mem->key)) {
        return V_STATUS_BUFFER_OVERFLOW;
    }
    vplatform_memcpy(mem->key, wire.key().data(), len);
    msg->key = mem->key;
    msg->key_len = (vuint32_t)len;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Load one attribute of @p wire into @p call and return its status.
 *
 * A server takes the value only on a verb that writes values, into slot @p i.
 * A client takes it only on a verb that does not, into its own buffers.
 *
 * @param[in]     call Call with class, verb, and the public message
 * @param[in]     src  Protobuf attribute
 * @param[in]     i    Attribute index
 * @param[in,out] mem  Server codec memory, or NULL on a client
 * @return The attribute status, or V_STATUS_INVALID_PARAMETER as a frame failure.
 */
static v_status_t drain_attr(vii_msg_t *call, const vedam::v1::Attr &src, int i,
                             struct vti_codec_mem *mem)
{
    const struct vii_attr_meta *m;
    v_attr_t *dst = &call->msg->attr_list[i];
    int writes = writes_values(call->verb);
    int take = mem != NULL ? writes : !writes;

    dst->id = src.id();
    m = attr_meta(call->class_id, dst->id);
    if (m == NULL) {
        return V_STATUS_SUCCESS;
    }
    if (mem != NULL) {
        slot_ready(m, writes, dst, mem->scratch[i]);
    }
    if (!take) {
        return V_STATUS_SUCCESS;
    }
    return take_attr_value(m, src.value(), dst);
}

/**
 * @brief Copy a protobuf Message into @p call.
 *
 * A server ignores the statuses in a request: each attribute is success, or
 * V_STATUS_BUFFER_OVERFLOW when its value does not fit its slot, and the
 * overall status is the first such failure. A client keeps the peer's statuses
 * and adds V_STATUS_BUFFER_OVERFLOW where a value does not fit its buffer.
 *
 * @param[in]     wire    Protobuf message
 * @param[in,out] call    Call whose message arrays are sized on entry
 * @param[out]    overall Overall status
 * @param[in,out] mem     Server codec memory, or NULL on a client
 * @return V_STATUS_SUCCESS, or a failure when counts or values are bad.
 */
static v_status_t drain_message(const vedam::v1::Message &wire, vii_msg_t *call,
                                v_status_t *overall, struct vti_codec_mem *mem)
{
    vi_msg_t *msg = call->msg;
    v_status_t first = V_STATUS_SUCCESS;
    v_status_t st;
    int n;
    int i;

    n = wire.attrs_size();
    if (n < 0 || (vuint32_t)n > msg->attr_count) {
        return V_STATUS_BUFFER_OVERFLOW;
    }
    if ((vuint32_t)n > 0 && (msg->attr_list == NULL || msg->status_list == NULL)) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (mem != NULL) {
        st = take_key(wire, msg, mem);
        if (st != V_STATUS_SUCCESS) {
            return st;
        }
    }
    for (i = 0; i < n; i++) {
        st = drain_attr(call, wire.attrs(i), i, mem);
        if (st == V_STATUS_INVALID_PARAMETER) {
            return st;
        }
        if (st != V_STATUS_SUCCESS && first == V_STATUS_SUCCESS) {
            first = st;
        }
        msg->status_list[i] =
            (mem == NULL && st == V_STATUS_SUCCESS) ? peer_status(wire.attrs(i), i) : st;
    }
    msg->attr_count = (vuint32_t)n;
    if (mem != NULL) {
        *overall = first;
        return V_STATUS_SUCCESS;
    }
    *overall = (v_status_t)wire.status().vedam_status();
    if (*overall == V_STATUS_SUCCESS && wire.status().code() != vedam::v1::STATUS_OK &&
        wire.status().code() != vedam::v1::STATUS_CODE_UNSPECIFIED) {
        *overall = from_proto_code(wire.status().code(), 0);
    }
    if (*overall == V_STATUS_SUCCESS) {
        *overall = first;
    }
    return V_STATUS_SUCCESS;
}

} /* namespace */

/**
 * @brief Allocate a codec workspace (Arena and one Message).
 *
 * @return Workspace pointer, or NULL when allocation fails.
 */
extern "C" vti_codec_ws_t *vti_codec_ws_create(void)
{
    std::unique_ptr<vti_codec_ws> ws(new (std::nothrow) vti_codec_ws());

    if (ws == nullptr || ws->wire == nullptr) {
        return nullptr;
    }
    return ws.release();
}

/**
 * @brief Free a codec workspace.
 *
 * @param[in] ws Workspace from vti_codec_ws_create, or NULL
 */
extern "C" void vti_codec_ws_destroy(vti_codec_ws_t *ws)
{
    std::unique_ptr<vti_codec_ws> owned(ws);
}

/**
 * @brief Map a library status to a grpc-web digit string.
 *
 * @param[in] st Library status
 * @return Static digit string for the HTTP trailer.
 */
extern "C" const char *vti_codec_grpc_digits(v_status_t st)
{
    if (st == V_STATUS_SUCCESS) {
        return "0";
    }
    if (st == V_STATUS_NOT_IMPLEMENTED) {
        return "12";
    }
    return "2";
}

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
extern "C" v_status_t vti_codec_grpc_frame(uint8_t *buf, size_t buf_cap, size_t payload_len,
                                           size_t *out_len)
{
    uint32_t n;

    if (buf == NULL || out_len == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (buf_cap < 5u + payload_len) {
        return V_STATUS_BUFFER_OVERFLOW;
    }
    n = htonl(static_cast<uint32_t>(payload_len));
    buf[0] = 0;
    std::memcpy(buf + 1, &n, 4);
    *out_len = 5u + payload_len;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Strip a five-byte grpc-web frame header when present.
 *
 * @param[in,out] body     Points at the body start
 * @param[in,out] body_len Body length
 */
extern "C" void vti_codec_grpc_unframe(const uint8_t **body, size_t *body_len)
{
    if (body == NULL || body_len == NULL || *body == NULL) {
        return;
    }
    if (*body_len >= 5 && (*body)[0] == 0) {
        *body += 5;
        *body_len -= 5;
    }
}

/**
 * @brief Method name for @p verb, or NULL when unknown.
 *
 * @param[in] verb Verb identifier
 * @return Static name string, or NULL.
 */
extern "C" const char *vti_codec_verb_name(v_verb_t verb)
{
    switch (verb) {
    case V_VERB_GET:
        return "Get";
    case V_VERB_SUBSCRIBE:
        return "Subscribe";
    case V_VERB_UNSUBSCRIBE:
        return "Unsubscribe";
    case V_VERB_CREATE:
        return "Create";
    case V_VERB_SET:
        return "Set";
    case V_VERB_DELETE:
        return "Delete";
    case V_VERB_NOTIFY:
        return "Notify";
    default:
        return NULL;
    }
}

/**
 * @brief Parse a method name into @p verb.
 *
 * @param[in]  name Method name
 * @param[out] verb Receives the verb id
 * @return V_STATUS_SUCCESS, or V_STATUS_NOT_IMPLEMENTED when unknown.
 */
extern "C" v_status_t vti_codec_verb_parse(const char *name, v_verb_t *verb)
{
    if (name == NULL || verb == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (std::strcmp(name, "Get") == 0) {
        *verb = V_VERB_GET;
        return V_STATUS_SUCCESS;
    }
    if (std::strcmp(name, "Subscribe") == 0) {
        *verb = V_VERB_SUBSCRIBE;
        return V_STATUS_SUCCESS;
    }
    if (std::strcmp(name, "Unsubscribe") == 0) {
        *verb = V_VERB_UNSUBSCRIBE;
        return V_STATUS_SUCCESS;
    }
    if (std::strcmp(name, "Create") == 0) {
        *verb = V_VERB_CREATE;
        return V_STATUS_SUCCESS;
    }
    if (std::strcmp(name, "Set") == 0) {
        *verb = V_VERB_SET;
        return V_STATUS_SUCCESS;
    }
    if (std::strcmp(name, "Delete") == 0) {
        *verb = V_VERB_DELETE;
        return V_STATUS_SUCCESS;
    }
    if (std::strcmp(name, "Notify") == 0) {
        *verb = V_VERB_NOTIFY;
        return V_STATUS_SUCCESS;
    }
    return V_STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Encode the message of @p call as a protobuf Message into @p buf.
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
extern "C" v_status_t vti_codec_encode(const vii_msg_t *call, v_status_t overall, int reply,
                                       uint8_t *buf, size_t cap, size_t *out_len,
                                       vti_codec_ws_t *ws)
{
    const vi_msg_t *msg;
    v_status_t st;
    size_t n;

    if (call == NULL || call->msg == NULL || buf == NULL || out_len == NULL || ws == NULL ||
        ws->wire == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    msg = call->msg;
    if (msg->attr_count > 0 && (msg->attr_list == NULL || msg->status_list == NULL)) {
        return V_STATUS_INVALID_PARAMETER;
    }

    ws->recycle();
    st = fill_message(call, overall, reply, ws->wire);
    if (st != V_STATUS_SUCCESS) {
        return st;
    }

    n = ws->wire->ByteSizeLong();
    if (n > cap) {
        return V_STATUS_BUFFER_OVERFLOW;
    }
    if (!ws->wire->SerializeToArray(buf, static_cast<int>(n))) {
        return V_STATUS_FAILURE;
    }
    *out_len = n;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Decode a protobuf Message into the message of @p call.
 *
 * @param[in]     buf     Encoded Message bytes
 * @param[in]     len     Length of @p buf
 * @param[in,out] call    Call with class and verb set, and the public message to fill
 * @param[out]    overall Overall status
 * @param[in,out] ws      Connection-owned codec workspace
 * @param[in,out] mem     Server codec memory prepared by vti_codec_rx_prepare, or NULL on a client
 * @return V_STATUS_SUCCESS, or a failure when the frame is bad.
 */
extern "C" v_status_t vti_codec_decode(const uint8_t *buf, size_t len, vii_msg_t *call,
                                       v_status_t *overall, vti_codec_ws_t *ws,
                                       struct vti_codec_mem *mem)
{
    if (call == NULL || call->msg == NULL || overall == NULL || ws == NULL || ws->wire == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (buf == NULL && len != 0) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (len > (size_t)INT32_MAX) {
        return V_STATUS_INVALID_PARAMETER;
    }

    ws->recycle();
    if (!ws->wire->ParseFromArray(buf, static_cast<int>(len))) {
        return V_STATUS_INVALID_PARAMETER;
    }
    return drain_message(*ws->wire, call, overall, mem);
}

/**
 * @brief Point @p msg at the attribute and status slots of @p mem and clear them.
 *
 * @param[in,out] mem Server codec memory
 * @param[out]    msg Server message to fill by vti_codec_decode
 */
extern "C" void vti_codec_rx_prepare(struct vti_codec_mem *mem, vi_msg_t *msg)
{
    vplatform_memset(msg, 0, sizeof(*msg));
    vplatform_memset(mem->attrs, 0, sizeof(mem->attrs));
    vplatform_memset(mem->statuses, 0, sizeof(mem->statuses));
    msg->attr_count = VTI_CODEC_MAX_ATTR;
    msg->attr_list = mem->attrs;
    msg->status_list = mem->statuses;
}

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
extern "C" int vti_codec_http(const char *service, const char *method, const uint8_t *body,
                              size_t body_len, uint8_t *out_buf, size_t out_cap, size_t *out_len,
                              const char **grpc_status, struct vti_codec_mem *mem)
{
    vi_msg_t vim;
    vii_msg_t call;
    v_class_id_t class_id;
    vuint32_t major;
    vuint32_t minor;
    v_verb_t verb;
    v_status_t st;
    v_status_t overall;
    size_t payload_len;
    size_t framed_len;
    int watched = 0;

    if (service == NULL || method == NULL || out_buf == NULL || out_len == NULL ||
        grpc_status == NULL || mem == NULL || mem->ws == NULL) {
        return -1;
    }
    if (body == NULL && body_len != 0) {
        return -1;
    }
    *out_len = 0;
    *grpc_status = "12";

    st = vti_service_lookup(service, &class_id, &major, &minor);
    if (st != V_STATUS_SUCCESS) {
        return 0;
    }

    st = vti_codec_verb_parse(method, &verb);
    if (st != V_STATUS_SUCCESS) {
        return 0;
    }

    vti_codec_rx_prepare(mem, &vim);
    vplatform_memset(&call, 0, sizeof(call));
    call.class_id = class_id;
    call.origin = V_ORIGIN_EXTERNAL;
    call.verb = verb;
    call.major = major;
    call.minor = minor;
    call.msg = &vim;

    st = vti_codec_decode(body == NULL ? (const uint8_t *)"" : body, body_len, &call, &overall,
                          mem->ws, mem);
    if (st != V_STATUS_SUCCESS) {
        return -1;
    }
    if (overall == V_STATUS_SUCCESS) {
        /* The reply encode reads the marks, so the watch stays up through it. */
        vattr_write_watch(mem->attrs, VTI_CODEC_MAX_ATTR);
        watched = 1;
        overall = vii_dispatch(&call);
    }

    if (out_cap < 5u) {
        if (watched) {
            vattr_write_unwatch();
        }
        return -1;
    }
    st = vti_codec_encode(&call, overall, 1, out_buf + 5, out_cap - 5u, &payload_len, mem->ws);
    if (watched) {
        vattr_write_unwatch();
    }
    if (st != V_STATUS_SUCCESS) {
        return -1;
    }

    st = vti_codec_grpc_frame(out_buf, out_cap, payload_len, &framed_len);
    if (st != V_STATUS_SUCCESS) {
        return -1;
    }
    *out_len = framed_len;
    *grpc_status = vti_codec_grpc_digits(overall);
    return 0;
}

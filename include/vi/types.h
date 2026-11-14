/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#ifndef VI_TYPES_H
#define VI_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t vuint8_t;
typedef int8_t vint8_t;
typedef uint16_t vuint16_t;
typedef int16_t vint16_t;
typedef uint32_t vuint32_t;
typedef int32_t vint32_t;
typedef uint64_t vuint64_t;
typedef int64_t vint64_t;
typedef vuint8_t vbool_t;

#define V_TRUE 1
#define V_FALSE 0

/** @brief Attribute identifier. */
typedef vuint32_t v_attr_id_t;

/**
 * @brief Status codes returned by Vedam verbs.
 */
typedef enum _v_status_t {
    V_STATUS_SUCCESS = 0, /**< The call completed and any output buffers are valid. */
    V_STATUS_FAILURE =
        -1, /**< The call failed for a reason not covered by a more specific code. Discard outputs. */
    V_STATUS_NOT_SUPPORTED =
        -2, /**< This build or origin does not support the verb or attribute. Discard outputs. */
    V_STATUS_NO_MEMORY =
        -3, /**< A buffer could not be allocated. Retry later or use a smaller list. */
    V_STATUS_INVALID_PARAMETER =
        -4, /**< A pointer, count, or length is not usable. Fix the arguments and retry. */
    V_STATUS_ITEM_ALREADY_EXISTS = -5, /**< Create was asked for an object that already exists. */
    V_STATUS_ITEM_NOT_FOUND = -6, /**< Read, set, or delete named an object that does not exist. */
    V_STATUS_BUFFER_OVERFLOW =
        -7, /**< A caller-supplied buffer is too small. The needed length is written when the type allows it. */
    V_STATUS_MANDATORY_ATTR_MISSING =
        -8, /**< Create lacked an attribute that must be present. Add it and retry. */
    V_STATUS_UNINITIALIZED =
        -9, /**< vedam_init has not completed, or vedam_deinit has already run. */
    V_STATUS_NOT_IMPLEMENTED =
        -10, /**< The verb exists in the header but this origin or slice must not call it. */
    V_STATUS_FAILED_PRECONDITION =
        -11, /**< The object exists but is not in a state that allows this verb. */
    V_STATUS_CAPABILITY_DENIED =
        -12, /**< The caller is not allowed to use this verb or attribute. */
    V_STATUS_OFFLINE = -13, /**< The path needs a network or peer that is not reachable. */
    V_STATUS_THERMAL_SHED = -14, /**< The device is too hot or too busy. Retry after a wait. */
    V_STATUS_OBJECT_IN_USE =
        -15, /**< Delete or set cannot proceed while another call holds the object. */
    V_STATUS_INVALID_ATTR_VALUE_0 =
        -100, /**< An attribute value is not valid. Add the zero-based index of that attribute in the list to this code. */
    V_STATUS_ATTR_NOT_IMPLEMENTED_0 =
        -200, /**< An attribute id is known but not implemented. Add the zero-based index of that attribute in the list to this code. */
    V_STATUS_UNKNOWN_ATTRIBUTE_0 =
        -300, /**< An attribute id is not defined for this class. Add the zero-based index of that attribute in the list to this code. */
    V_STATUS_ATTR_NOT_SUPPORTED_0 =
        -400, /**< The origin may not use this attribute on this verb. Add the zero-based index of that attribute in the list to this code. */
    V_STATUS_INVALID_ATTR_ID_0 =
        -500, /**< An attribute id is out of range. Add the zero-based index of that attribute in the list to this code. */
} v_status_t;

/**
 * @brief Protocol or library version pair.
 */
typedef struct _v_version_t {
    vuint32_t major;
    vuint32_t minor;
} v_version_t;

/**
 * @brief UTF-8 text with explicit length.
 *
 * On get the caller points @p buf at its buffer and sets @p len to the room in
 * it. The reply sets @p len to the bytes written. When the text does not fit,
 * that attribute's status is V_STATUS_BUFFER_OVERFLOW, nothing is copied, and
 * @p len holds the needed length.
 */
typedef struct _v_text_t {
    vuint32_t len;
    char *buf;
} v_text_t;

/**
 * @brief Items of one type, laid out one after another, with a count.
 *
 * The attribute names the item type. On get the caller points @p items at its
 * array and sets @p count to the room in it. The reply sets @p count to the
 * items written. When the list does not fit, that attribute's status is
 * V_STATUS_BUFFER_OVERFLOW, nothing is copied, and @p count holds the needed
 * number.
 */
typedef struct _v_list_t {
    vuint32_t count;
    void *items;
} v_list_t;

/**
 * @brief Attribute value payload. The active member depends on the attribute type.
 */
typedef union _v_attr_value_t {
    vbool_t booldata;
    vuint8_t u8;
    vint8_t s8;
    vuint16_t u16;
    vint16_t s16;
    vuint32_t u32;
    vint32_t s32;
    vuint64_t u64;
    vint64_t s64;
    v_version_t version;
    v_text_t text;
    v_list_t list;
    void *ptr;
} v_attr_value_t;

/**
 * @brief One attribute: identifier plus value.
 */
typedef struct _v_attr_t {
    v_attr_id_t id;
    v_attr_value_t value;
} v_attr_t;

/**
 * @brief One public call: object key, attributes, and a status per attribute.
 *
 * A singleton sets @p key to NULL and @p key_len to 0. When @p attr_count is
 * greater than zero, both @p attr_list and @p status_list must be non-NULL.
 * Contract checks write every @p status_list entry. The verb returns success
 * only when every entry is success; otherwise it returns the first non-success
 * code.
 */
typedef struct _vi_msg_t {
    const void *key;
    vuint32_t key_len;
    vuint32_t attr_count;
    v_attr_t *attr_list;
    v_status_t *status_list;
} vi_msg_t;

/**
 * @brief Namespace for Vedam's own vertical public ids (16 bytes).
 *
 * Vedam hashes vertical/major/minor/commit under this namespace to form a
 * version-5 UUID. Another author may use that recipe with their own domain, or
 * may publish sixteen bytes produced another way.
 */
static const uint8_t vi_namespace_uuid[16] = {
    0x04, 0xd4, 0xb1, 0xcf, 0x54, 0xb6, 0x53, 0xb2, 0x82, 0x81, 0x5a, 0xf9, 0xb0, 0x4a, 0x66, 0x66,
};

#ifdef __cplusplus
}
#endif

#endif /* VI_TYPES_H */

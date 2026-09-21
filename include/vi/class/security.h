/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#ifndef VI_CLASS_SECURITY_H
#define VI_CLASS_SECURITY_H

#include "vi/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Attribute identifiers for security.
 *
 * Public attributes only.
 */
typedef enum _vi_security_attr_t {
    VI_SECURITY_ATTR_UNSPECIFIED = 0,
    /**
     * @brief Active security profile for this process.
     * @type vint32_t
     * @flags READ_ONLY
     */
    VI_SECURITY_ATTR_PROFILE = 1,
    /**
     * @brief State of the security singleton.
     * @type vint32_t
     * @flags READ_ONLY | SUBSCRIBABLE
     */
    VI_SECURITY_ATTR_STATE = 2,
} vi_security_attr_t;

/**
 * @brief Public id for this vertical (16 bytes).
 *
 * Vedam's own verticals use UUID version 5 of vertical/major/minor/commit under
 * the namespace in vi/types.h. The commit is the constant that opened that
 * major.minor line; a later line does not rewrite it. A hyphenated form for logs
 * or HTTP is derived from these bytes when a message leaves this process.
 */
static const uint8_t vi_security_uuid[16] = { 0x3d, 0x8d, 0x56, 0x23, 0x8c, 0xa8, 0x58, 0x5f, 0xa4, 0x34, 0x66, 0x4c, 0x1d, 0xf3, 0x8f, 0xfa };

/**
 * @brief Fill the values for the attribute ids in the message.
 *
 * The caller sets each id. For a state attribute the caller sets value.ptr.
 * Duplicate ids keep the last entry. A singleton sets key to NULL and key_len
 * to 0. When attr_count is greater than zero, both attr_list and status_list
 * must be set. Each status_list entry is written; the overall return is success
 * only when every entry is success.
 *
 * @param[in,out] msg Call message
 * @return The local-transport status.
 */
v_status_t vi_security_get(vi_msg_t *msg);

/**
 * @brief Register a watch for attributes marked subscribable. This call returns once.
 *
 * A singleton sets key to NULL and key_len to 0. When attr_count is greater
 * than zero, both attr_list and status_list must be set.
 *
 * @param[in,out] msg Call message
 * @return The local-transport status.
 */
v_status_t vi_security_subscribe(vi_msg_t *msg);

/**
 * @brief Stop watches for the same caller. An empty list stops every watch.
 *
 * A singleton sets key to NULL and key_len to 0. When attr_count is greater
 * than zero, both attr_list and status_list must be set.
 *
 * @param[in,out] msg Call message
 * @return The local-transport status.
 */
v_status_t vi_security_unsubscribe(vi_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* VI_CLASS_SECURITY_H */

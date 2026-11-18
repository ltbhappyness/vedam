/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#ifndef VI_CLASS_SYSTEM_H
#define VI_CLASS_SYSTEM_H

#include "vi/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief vi_system_node_class_t values for the system class.
 */
typedef enum _vi_system_node_class_t {
    VI_SYSTEM_NODE_CLASS_UNSPECIFIED = 0, /**< No role has been chosen. */
    VI_SYSTEM_NODE_CLASS_A = 1, /**< Full host libraries, all ordinary links, the daemon. It is not a different call shape. */
    VI_SYSTEM_NODE_CLASS_B = 2, /**< The same host shape as A. It is not a second protocol. */
    VI_SYSTEM_NODE_CLASS_C = 3, /**< The default host: queue, TCP, and the local socket, plus the daemon. It does not include the compiler path. */
    VI_SYSTEM_NODE_CLASS_D = 4, /**< Host libraries with the byte links off. It does not listen on a queue or a socket. */
    VI_SYSTEM_NODE_CLASS_E = 5, /**< A WebAssembly build of the libraries. No daemon, no lock, no byte links. It does not run on the machine that merely hosts the page. */
} vi_system_node_class_t;

/**
 * @brief vi_system_state_e values for the system class.
 */
typedef enum _vi_system_state_e {
    VI_SYSTEM_STATE_UNSPECIFIED = 0, /**< State has not been set. */
    VI_SYSTEM_STATE_STARTING = 1, /**< Create has started. Get is not ready. */
    VI_SYSTEM_STATE_READY = 2, /**< Transports are up. Get and subscribe are allowed. */
    VI_SYSTEM_STATE_DEGRADED = 3, /**< The process is up with a reduced set of transports. */
    VI_SYSTEM_STATE_DRAINING = 4, /**< The process is finishing work and will stop. */
    VI_SYSTEM_STATE_DEAD = 5, /**< The process has stopped. Get returns uninitialized. */
} vi_system_state_e;

/**
 * @brief vi_system_state_t for the system class.
 */
typedef struct _vi_system_state_t {
    vi_system_state_e operational; /**< Where the process is in its life. */
    vuint32_t reserved[8]; /**< Kept zero by the node. */
} vi_system_state_t;

/**
 * @brief Attribute identifiers for system.
 *
 * Public attributes only.
 */
typedef enum _vi_system_attr_t {
    VI_SYSTEM_ATTR_UNSPECIFIED = 0,
    /**
     * @brief The letter this build was compiled as. Not a runtime job title.
     * @type vi_system_node_class_t
     * @values vi_system_node_class_t
     * @flags READ_ONLY
     */
    VI_SYSTEM_ATTR_NODE_CLASS = 1,
    /**
     * @brief Agreed message version for this node. This era always reports major 0 and minor 1.
     * @type v_version_t
     * @flags READ_ONLY | SUBSCRIBABLE
     */
    VI_SYSTEM_ATTR_PROTOCOL_VERSION = 2,
    /**
     * @brief Whether the process is starting, ready, degraded, draining, or dead. Pass value.ptr to a caller-owned vi_system_state_t.
     * @type vi_system_state_t
     * @flags READ_ONLY | SUBSCRIBABLE
     */
    VI_SYSTEM_ATTR_SYSTEM_STATE = 3,
    /**
     * @brief Commit of the running build, for a person to read. Not an input to the vertical UUID.
     * @type v_text_t
     * @get Set len to the room in buf. On overflow len holds the needed length.
     * @flags READ_ONLY
     */
    VI_SYSTEM_ATTR_RELEASE_COMMIT = 4,
} vi_system_attr_t;

/**
 * @brief Public id for this vertical (16 bytes).
 *
 * Vedam's own verticals use UUID version 5 of vertical/major/minor/commit under
 * the namespace in vi/types.h. The commit is the constant that opened that
 * major.minor line; a later line does not rewrite it. A hyphenated form for logs
 * or HTTP is derived from these bytes when a message leaves this process.
 */
static const uint8_t vi_system_uuid[16] = {
    0xc2, 0xb9, 0x15, 0x80, 0x60, 0x92, 0x5c, 0x64, 0xbd, 0x0c, 0xea, 0x7f, 0x37, 0x7b, 0x56, 0xc1,
};

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
 * @return The call status.
 */
v_status_t vi_system_get(vi_msg_t *msg);

/**
 * @brief Register a watch for attributes marked subscribable. This call returns once.
 *
 * A singleton sets key to NULL and key_len to 0. When attr_count is greater
 * than zero, both attr_list and status_list must be set.
 *
 * @param[in,out] msg Call message
 * @return The call status.
 */
v_status_t vi_system_subscribe(vi_msg_t *msg);

/**
 * @brief Stop watches for the same caller. An empty list stops every watch.
 *
 * A singleton sets key to NULL and key_len to 0. When attr_count is greater
 * than zero, both attr_list and status_list must be set.
 *
 * @param[in,out] msg Call message
 * @return The call status.
 */
v_status_t vi_system_unsubscribe(vi_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* VI_CLASS_SYSTEM_H */

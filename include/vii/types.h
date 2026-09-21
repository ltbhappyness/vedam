/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#ifndef VII_TYPES_H
#define VII_TYPES_H

#include "vi/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Call origin. External is the public VI path; internal is platform code.
 */
typedef enum _v_origin_t {
    V_ORIGIN_EXTERNAL = 0,
    V_ORIGIN_INTERNAL = 1
} v_origin_t;

/**
 * @brief Stable class identifiers used by dispatch and transports.
 */
typedef enum _v_class_id_t {
    V_CLASS_UNSPECIFIED = 0,
    V_CLASS_SYSTEM = 1, /**< The process system object. */
    V_CLASS_IDENTITY = 2, /**< Identity objects keyed by the caller object id. */
    V_CLASS_STORAGE = 3, /**< Storage objects keyed by the caller object id. */
    V_CLASS_SECURITY = 4, /**< The process security singleton. */
    V_CLASS_WASM = 5, /**< The process WebAssembly configuration singleton. */
    V_CLASS_ID_MAX = 5
} v_class_id_t;

/**
 * @brief Verb identifiers shared by VII and transport call paths.
 */
typedef enum _v_verb_t {
    V_VERB_CREATE = 0,
    V_VERB_GET = 1,
    V_VERB_SET = 2,
    V_VERB_DELETE = 3,
    V_VERB_SUBSCRIBE = 4,
    V_VERB_UNSUBSCRIBE = 5,
    V_VERB_NOTIFY = 6
} v_verb_t;

/**
 * @brief One message on a transport descriptor.
 *
 * Create, get, set, delete, subscribe, unsubscribe, and notify are @p verb.
 * @p msg holds the object key, attributes, and per-attribute status codes. The
 * body writes values into @p msg->attr_list and codes into @p msg->status_list.
 * @p major and @p minor come from the UUID line on IPC and remote, or from the
 * latest constants on a local call.
 */
typedef struct vta_msg {
    v_class_id_t class_id;
    v_origin_t origin;
    v_verb_t verb;
    vuint32_t major;
    vuint32_t minor;
    vi_msg_t *msg;
} vta_msg_t;

typedef struct vta_desc vta_desc_t;

/**
 * @brief Hooks for one concrete transport. Private state is vta_desc_t.ctx.
 */
typedef struct vta_ops {
    v_status_t (*close)(vta_desc_t *desc);
    v_status_t (*listen)(vta_desc_t *desc, const char *address);
    v_status_t (*accept)(vta_desc_t *desc, vta_desc_t **client);
    v_status_t (*connect)(vta_desc_t *desc, const char *address);
    v_status_t (*call)(vta_desc_t *desc, vta_msg_t *msg);
} vta_ops_t;

/**
 * @brief Socket-like transport endpoint.
 *
 * The caller that opened the descriptor closes it. Subscribe does not store
 * it on the node.
 */
struct vta_desc {
    const vta_ops_t *ops;
    void *ctx;
    int link;
};

#ifdef __cplusplus
}
#endif

#endif /* VII_TYPES_H */

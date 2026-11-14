/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VTI_VTI_H
#define VTI_VTI_H

#include "vii/types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { VTI_LINK_LOCAL = 0, VTI_LINK_IPC = 1, VTI_LINK_REMOTE = 2, VTI_LINK_COUNT = 3 };

typedef struct vti_desc vti_desc_t;

/**
 * @brief Hooks for one concrete transport. Private state is vti_desc_t.ctx.
 */
typedef struct vti_ops {
    v_status_t (*close)(vti_desc_t *desc);
    v_status_t (*listen)(vti_desc_t *desc, const char *address);
    v_status_t (*accept)(vti_desc_t *desc, vti_desc_t **client);
    v_status_t (*connect)(vti_desc_t *desc, const char *address);
    v_status_t (*call)(vti_desc_t *desc, vii_msg_t *msg);
} vti_ops_t;

/**
 * @brief Socket-like transport endpoint.
 *
 * The caller that opened the descriptor closes it. Subscribe does not store
 * it on the node.
 */
struct vti_desc {
    const vti_ops_t *ops;
    void *ctx;
    int link;
};

/**
 * @brief Empty descriptor for @p link. The node must already exist.
 *
 * The operations table is the one chosen for that link at vedam_init.
 * The address is applied later by vti_listen or vti_connect.
 */
vti_desc_t *vti_open(int link);

/** @brief Close @p desc. The caller that opened it closes it. */
void vti_close(vti_desc_t *desc);

/** @brief Bind @p desc and wait for callers. */
v_status_t vti_listen(vti_desc_t *desc, const char *address);

/** @brief Accept one caller. The listen thread owns this for byte transports. */
v_status_t vti_accept(vti_desc_t *desc, vti_desc_t **client);

/** @brief Dial @p address. The result is a sender descriptor. */
v_status_t vti_connect(vti_desc_t *desc, const char *address);

/**
 * @brief One verb on @p desc.
 *
 * Runs the verb and writes values and per-attribute status codes into
 * @p msg->msg. Does not close @p desc; the caller that opened it closes it.
 */
v_status_t vti_call(vti_desc_t *desc, vii_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* VTI_VTI_H */

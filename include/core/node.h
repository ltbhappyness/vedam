/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VEDAM_NODE_H
#define VEDAM_NODE_H

#include "lock/store.h"
#include "vti/vti.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vedam_lockdb;
struct vds_hash;
struct vii_class;

/**
 * @brief Process-wide node.
 *
 * @p rx and @p ops are indexed by VTI_LINK_LOCAL, VTI_LINK_IPC, and
 * VTI_LINK_REMOTE. Receivers stay open for the life of the node. @p ops
 * is how vti_open picks a table. Sender descriptors are not a fixed set
 * here; the caller that opened a sender closes it. @p uuid_map maps a
 * 16-byte vertical id to class index, major, and minor for IPC.
 * @p service_map maps a gRPC service name the same way for remote.
 * @p uuid_hits and @p service_hits hold the values allocated with those maps.
 * @p classes is the class table, indexed by class id.
 */
typedef struct vedam_node {
    vti_desc_t *rx[VTI_LINK_COUNT];
    const vti_ops_t *ops[VTI_LINK_COUNT];
    struct vedam_lockdb *locks;
    struct vds_hash *uuid_map;
    void *uuid_hits;
    struct vds_hash *service_map;
    void *service_hits;
    const struct vii_class *classes;
} vedam_t;

/**
 * @brief The node allocated by vedam_init, or NULL before init and after deinit.
 *
 * Only vedam_init and vedam_deinit write it.
 */
extern vedam_t *g_vedam_node;

/**
 * @brief Return the process node, or NULL before init and after deinit.
 *
 * @return Pointer to the process node, or NULL.
 */
static inline vedam_t *vedam_node(void)
{
    return g_vedam_node;
}

/**
 * @brief Send @p msg on @p desc. A library body sends only through this.
 *
 * A non-local send waits until the Wait-Die attempt on this thread has
 * released its locks: while an attempt is held it is refused. The
 * in-process hook re-enters the node and joins that attempt.
 *
 * @param[in]     desc Transport descriptor
 * @param[in,out] msg  Call and reply message
 * @return The transport status, V_STATUS_INVALID_PARAMETER, or
 *         V_STATUS_FAILED_PRECONDITION while an attempt is held.
 */
static inline v_status_t vedam_node_call(vti_desc_t *desc, vii_msg_t *msg)
{
    if (desc == NULL || desc->ops == NULL || desc->ops->call == NULL || msg == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (desc->link != VTI_LINK_LOCAL && vedam_lock_in_attempt()) {
        return V_STATUS_FAILED_PRECONDITION;
    }
    return desc->ops->call(desc, msg);
}

#ifdef __cplusplus
}
#endif

#endif /* VEDAM_NODE_H */

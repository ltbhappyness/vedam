/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VEDAM_CORE_H
#define VEDAM_CORE_H

#include "vti/vta.h"
#include "vti/dispatch.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vedam_lockdb;
struct vds_hash;

/**
 * @brief Process-wide node.
 *
 * @p rx and @p ops are indexed by VTA_LINK_LOCAL, VTA_LINK_IPC, and
 * VTA_LINK_REMOTE. Receivers stay open for the life of the node. @p ops
 * is how vta_open picks a table. Sender descriptors are not a fixed set
 * here; the caller that opened a sender closes it. @p uuid_map maps a
 * 16-byte vertical id to class index, major, and minor for IPC and remote.
 * @p uuid_hits holds the hash values allocated with that map.
 * @p verbs is the class-indexed verb table filled at init.
 */
typedef struct vedam_node {
    vta_desc_t *rx[VTA_LINK_COUNT];
    const vta_ops_t *ops[VTA_LINK_COUNT];
    struct vedam_lockdb *locks;
    struct vds_hash *uuid_map;
    void *uuid_hits;
    struct vta_class_verbs *verbs;
} vedam_t;

/**
 * @brief The node allocated by vedam_init, or NULL before init and after deinit.
 */
vedam_t *vedam_node(void);

#ifdef __cplusplus
}
#endif

#endif

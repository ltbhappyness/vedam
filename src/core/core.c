/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "core/node.h"

#include "lock/store.h"
#include "platform/platform.h"
#include "util/log.h"
#include "vi/vedam.h"
#include "vii/class/system.h"
#include "vii/dispatch.h"
#include "vii/meta.h"
#include "vti/class_uuid.h"
#include "vti/connect.h"
#include "vti/grpc.h"
#include "vti/grpc_uds.h"
#include "vti/inproc.h"
#include "vti/mq.h"
#include "vti/vti.h"

#include <stdio.h>
#include <string.h>

vedam_t *g_vedam_node;

/**
 * @brief Map a transport name to its operations table.
 *
 * @param[in] name Transport name
 * @return The operations table, or NULL when the name is unknown.
 */
static const vti_ops_t *ops_by_name(const char *name)
{
    if (strcmp(name, "inproc") == 0) {
        return &vti_inproc_ops;
    }

    if (strcmp(name, "mq") == 0) {
        return &vti_mq_ops;
    }

    if (strcmp(name, "grpc") == 0) {
        return &vti_grpc_ops;
    }

    if (strcmp(name, "grpc_uds") == 0) {
        return &vti_grpc_uds_ops;
    }

    if (strcmp(name, "connect") == 0) {
        return &vti_connect_ops;
    }

    return NULL;
}

/**
 * @brief Read an environment variable, or @p fallback when it is unset.
 *
 * @param[in] env_name Variable name
 * @param[in] fallback Value used when the variable is unset
 * @return The selected string.
 */
static const char *pick(const char *env_name, const char *fallback)
{
    const char *value = vplatform_getenv(env_name);

    if (value == NULL) {
        return fallback;
    }
    return value;
}

/**
 * @brief Open a receiver and listen on @p address.
 *
 * @param[out] out     Receives the listening descriptor
 * @param[in]  link    Transport link index
 * @param[in]  address Listen address, or NULL when the link has none
 * @return V_STATUS_SUCCESS, V_STATUS_NO_MEMORY, or the listen status.
 */
static v_status_t boot_rx(vti_desc_t **out, int link, const char *address)
{
    vti_desc_t *desc;
    v_status_t st;

    desc = vti_open(link);
    if (desc == NULL) {
        return V_STATUS_NO_MEMORY;
    }

    st = vti_listen(desc, address);
    if (st != V_STATUS_SUCCESS) {
        vlog_error("receiver listen failed %d", (int)st);
        vti_close(desc);
        return st;
    }

    *out = desc;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Allocate the node and start receive paths, then run internal create.
 *
 * @return V_STATUS_SUCCESS, or a failure status when setup cannot finish.
 */
v_status_t vedam_init(void)
{
    const char *kind[VTI_LINK_COUNT];
    const char *addr[VTI_LINK_COUNT];
    const char *remote_address;
    const char *ipc_address;
    const char *remote_kind;
    const char *ipc_kind;
    const char *local_kind;
    char remote_fallback[64];
    char uds_fallback[64];
    char ipc_fallback[64];
    vedam_t *node;
    v_status_t rc;
    int n;
    int i;

    if (g_vedam_node != NULL) {
        return V_STATUS_ITEM_ALREADY_EXISTS;
    }

    n = snprintf(ipc_fallback, sizeof(ipc_fallback), "/vedam.%ld", vplatform_pid());
    if (n < 0 || (size_t)n >= sizeof(ipc_fallback)) {
        return V_STATUS_FAILURE;
    }

    n = snprintf(uds_fallback, sizeof(uds_fallback), "/tmp/vedamd.%ld.sock", vplatform_pid());
    if (n < 0 || (size_t)n >= sizeof(uds_fallback)) {
        return V_STATUS_FAILURE;
    }

    n = snprintf(remote_fallback, sizeof(remote_fallback), "127.0.0.1:0");
    if (n < 0 || (size_t)n >= sizeof(remote_fallback)) {
        return V_STATUS_FAILURE;
    }

    local_kind = pick("VEDAM_TRANSPORT_LOCAL", "inproc");
    ipc_kind = pick("VEDAM_TRANSPORT_IPC", "mq");
    remote_kind = pick("VEDAM_TRANSPORT_REMOTE", "grpc");
    if (strcmp(ipc_kind, "grpc_uds") == 0) {
        ipc_address = pick("VEDAM_TRANSPORT_IPC_ADDR", uds_fallback);
    } else {
        ipc_address = pick("VEDAM_TRANSPORT_IPC_ADDR", ipc_fallback);
    }

    if (strcmp(remote_kind, "connect") == 0) {
        remote_address = pick("VEDAM_TRANSPORT_REMOTE_ADDR", "");
    } else {
        remote_address = pick("VEDAM_TRANSPORT_REMOTE_ADDR", remote_fallback);
    }

    kind[VTI_LINK_LOCAL] = local_kind;
    kind[VTI_LINK_IPC] = ipc_kind;
    kind[VTI_LINK_REMOTE] = remote_kind;
    addr[VTI_LINK_LOCAL] = NULL;
    addr[VTI_LINK_IPC] = ipc_address;
    addr[VTI_LINK_REMOTE] = remote_address;

    node = vplatform_calloc(1, sizeof(*node));
    if (node == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    node->classes = vii_classes;

    for (i = 0; i < VTI_LINK_COUNT; i++) {
        node->ops[i] = ops_by_name(kind[i]);
        if (node->ops[i] != NULL) {
            continue;
        }

        vlog_error("unknown transport %s", kind[i]);
        rc = V_STATUS_INVALID_PARAMETER;
        goto out_free_node;
    }

    g_vedam_node = node;
    rc = vedam_lockdb_open(node);
    if (rc != V_STATUS_SUCCESS) {
        goto out_clear_node;
    }

    rc = vti_class_uuid_map_init();
    if (rc != V_STATUS_SUCCESS) {
        goto out_close_locks;
    }

    for (i = 0; i < VTI_LINK_COUNT; i++) {
        rc = boot_rx(&node->rx[i], i, addr[i]);
        if (rc != V_STATUS_SUCCESS) {
            goto out_close_rx;
        }
    }

    vlog_info("local=%s ipc=%s remote=%s", local_kind, ipc_kind, remote_kind);
    {
        vi_msg_t vim;
        vii_msg_t call;

        vplatform_memset(&vim, 0, sizeof(vim));
        vplatform_memset(&call, 0, sizeof(call));
        call.class_id = V_CLASS_SYSTEM;
        call.origin = V_ORIGIN_INTERNAL;
        call.verb = V_VERB_CREATE;
        call.major = VII_SYSTEM_MAJOR;
        call.minor = VII_SYSTEM_MINOR;
        call.msg = &vim;
        rc = vii_dispatch(&call);
    }
    if (rc != V_STATUS_SUCCESS) {
        goto out_close_rx;
    }

    return V_STATUS_SUCCESS;

out_close_rx:
    while (i-- > 0) {
        vti_close(node->rx[i]);
    }
    vti_class_uuid_map_fini();
out_close_locks:
    vedam_lockdb_close(node);
out_clear_node:
    g_vedam_node = NULL;
out_free_node:
    vplatform_free(node);
    return rc;
}

/**
 * @brief Delete the system object, stop receive paths, and free the node.
 */
v_status_t vedam_deinit(void)
{
    vedam_t *node;
    int i;

    if (g_vedam_node == NULL) {
        return V_STATUS_UNINITIALIZED;
    }

    {
        vi_msg_t vim;
        vii_msg_t call;

        vplatform_memset(&vim, 0, sizeof(vim));
        vplatform_memset(&call, 0, sizeof(call));
        call.class_id = V_CLASS_SYSTEM;
        call.origin = V_ORIGIN_INTERNAL;
        call.verb = V_VERB_DELETE;
        call.major = VII_SYSTEM_MAJOR;
        call.minor = VII_SYSTEM_MINOR;
        call.msg = &vim;
        (void)vii_dispatch(&call);
    }
    node = g_vedam_node;
    for (i = VTI_LINK_COUNT; i-- > 0;) {
        vti_close(node->rx[i]);
    }
    vti_class_uuid_map_fini();
    vedam_lockdb_close(node);
    g_vedam_node = NULL;
    vplatform_free(node);
    return V_STATUS_SUCCESS;
}

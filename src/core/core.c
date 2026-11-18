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
#include "vti/inproc.h"
#include "vti/vti.h"
#if VEDAM_ENABLE_CONNECT
#include "vti/connect.h"
#endif
#if VEDAM_ENABLE_GRPC
#include "vti/grpc.h"
#endif
#if VEDAM_ENABLE_GRPC_UDS
#include "vti/grpc_uds.h"
#endif
#if VEDAM_ENABLE_MQ
#include "vti/mq.h"
#endif

#if VEDAM_ENABLE_GRPC || VEDAM_ENABLE_GRPC_UDS || VEDAM_ENABLE_MQ
#include <stdio.h>
#endif

vedam_t *g_vedam_node;

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
    vedam_t *node;
    v_status_t rc;
    int i;
#if VEDAM_ENABLE_GRPC || VEDAM_ENABLE_GRPC_UDS || VEDAM_ENABLE_MQ
    int n;
#endif
#if VEDAM_ENABLE_MQ
    char ipc_fallback[64];
    const char *ipc_address;
#elif VEDAM_ENABLE_GRPC_UDS
    char uds_fallback[64];
    const char *ipc_address;
#endif
#if VEDAM_ENABLE_GRPC
    char remote_fallback[64];
    const char *remote_address;
#elif VEDAM_ENABLE_CONNECT
    const char *remote_address;
#endif

    if (g_vedam_node != NULL) {
        return V_STATUS_ITEM_ALREADY_EXISTS;
    }

#if VEDAM_ENABLE_MQ
    n = snprintf(ipc_fallback, sizeof(ipc_fallback), "/vedam.%ld", vplatform_pid());
    if (n < 0 || (size_t)n >= sizeof(ipc_fallback)) {
        return V_STATUS_FAILURE;
    }
    ipc_address = ipc_fallback;
#elif VEDAM_ENABLE_GRPC_UDS
    n = snprintf(uds_fallback, sizeof(uds_fallback), "/tmp/vedamd.%ld.sock", vplatform_pid());
    if (n < 0 || (size_t)n >= sizeof(uds_fallback)) {
        return V_STATUS_FAILURE;
    }
    ipc_address = uds_fallback;
#endif /* VEDAM_ENABLE_MQ || VEDAM_ENABLE_GRPC_UDS */

#if VEDAM_ENABLE_GRPC
    n = snprintf(remote_fallback, sizeof(remote_fallback), "127.0.0.1:0");
    if (n < 0 || (size_t)n >= sizeof(remote_fallback)) {
        return V_STATUS_FAILURE;
    }
    remote_address = remote_fallback;
#elif VEDAM_ENABLE_CONNECT
    remote_address = "";
#endif /* VEDAM_ENABLE_GRPC || VEDAM_ENABLE_CONNECT */

    node = vplatform_calloc(1, sizeof(*node));
    if (node == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    node->classes = vii_classes;
    node->ops[VTI_LINK_LOCAL] = &vti_inproc_ops;
#if VEDAM_ENABLE_MQ
    node->ops[VTI_LINK_IPC] = &vti_mq_ops;
#elif VEDAM_ENABLE_GRPC_UDS
    node->ops[VTI_LINK_IPC] = &vti_grpc_uds_ops;
#endif /* VEDAM_ENABLE_MQ || VEDAM_ENABLE_GRPC_UDS */
#if VEDAM_ENABLE_GRPC
    node->ops[VTI_LINK_REMOTE] = &vti_grpc_ops;
#elif VEDAM_ENABLE_CONNECT
    node->ops[VTI_LINK_REMOTE] = &vti_connect_ops;
#endif /* VEDAM_ENABLE_GRPC || VEDAM_ENABLE_CONNECT */

    g_vedam_node = node;
    rc = vedam_lockdb_open(node);
    if (rc != V_STATUS_SUCCESS) {
        goto out_clear_node;
    }

    rc = vti_class_uuid_map_init();
    if (rc != V_STATUS_SUCCESS) {
        goto out_close_locks;
    }

    rc = boot_rx(&node->rx[VTI_LINK_LOCAL], VTI_LINK_LOCAL, NULL);
    if (rc != V_STATUS_SUCCESS) {
        i = VTI_LINK_LOCAL;
        goto out_close_rx;
    }

#if VEDAM_ENABLE_MQ || VEDAM_ENABLE_GRPC_UDS
    rc = boot_rx(&node->rx[VTI_LINK_IPC], VTI_LINK_IPC, ipc_address);
    if (rc != V_STATUS_SUCCESS) {
        i = VTI_LINK_IPC;
        goto out_close_rx;
    }
#endif /* VEDAM_ENABLE_MQ || VEDAM_ENABLE_GRPC_UDS */

#if VEDAM_ENABLE_GRPC || VEDAM_ENABLE_CONNECT
    rc = boot_rx(&node->rx[VTI_LINK_REMOTE], VTI_LINK_REMOTE, remote_address);
    if (rc != V_STATUS_SUCCESS) {
        i = VTI_LINK_REMOTE;
        goto out_close_rx;
    }
#endif /* VEDAM_ENABLE_GRPC || VEDAM_ENABLE_CONNECT */

    vlog_info("ipc=%d remote=%d", node->ops[VTI_LINK_IPC] != NULL,
              node->ops[VTI_LINK_REMOTE] != NULL);
    i = VTI_LINK_COUNT;
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

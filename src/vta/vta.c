/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "vti/vti.h"

#include "core/node.h"
#include "platform/platform.h"

/**
 * @brief Allocate an empty transport descriptor for @p link.
 *
 * @param[in] link VTI_LINK_LOCAL, VTI_LINK_IPC, or VTI_LINK_REMOTE
 * @return A new descriptor, or NULL when the node is missing or allocation fails.
 */
vti_desc_t *vti_open(int link)
{
    const vti_ops_t *ops = NULL;
    vti_desc_t *desc;
    vedam_t *node;

    node = vedam_node();
    if (node == NULL || (unsigned)link >= VTI_LINK_COUNT) {
        return NULL;
    }

    ops = node->ops[link];
    if (ops == NULL) {
        return NULL;
    }

    desc = vplatform_calloc(1, sizeof(*desc));
    if (desc == NULL) {
        return NULL;
    }

    desc->ops = ops;
    desc->link = link;
    return desc;
}

/**
 * @brief Close @p desc and free it.
 *
 * @param[in] desc Transport descriptor
 */
void vti_close(vti_desc_t *desc)
{
    if (desc == NULL) {
        return;
    }
    if (desc->ops != NULL && desc->ops->close != NULL) {
        (void)desc->ops->close(desc);
    }
    vplatform_free(desc);
}

/**
 * @brief Bind @p desc and wait for callers.
 *
 * @param[in] desc    Transport descriptor
 * @param[in] address Listen address for this link
 * @return The transport listen status.
 */
v_status_t vti_listen(vti_desc_t *desc, const char *address)
{
    if (desc == NULL || desc->ops == NULL || desc->ops->listen == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    return desc->ops->listen(desc, address);
}

/**
 * @brief Accept one caller on a listen descriptor.
 *
 * @param[in]  desc   Listen descriptor
 * @param[out] client Receives the accepted descriptor
 * @return The transport accept status.
 */
v_status_t vti_accept(vti_desc_t *desc, vti_desc_t **client)
{
    if (desc == NULL || desc->ops == NULL || desc->ops->accept == NULL || client == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    return desc->ops->accept(desc, client);
}

/**
 * @brief Dial @p address on @p desc.
 *
 * @param[in] desc    Transport descriptor
 * @param[in] address Peer address
 * @return The transport connect status.
 */
v_status_t vti_connect(vti_desc_t *desc, const char *address)
{
    if (desc == NULL || desc->ops == NULL || desc->ops->connect == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    return desc->ops->connect(desc, address);
}

/**
 * @brief Run one verb on @p desc using a single call/reply message.
 *
 * A non-local descriptor is refused while a Wait-Die attempt is held.
 * The send runs after that attempt releases its locks.
 *
 * @param[in]     desc Transport descriptor
 * @param[in,out] msg  Call and reply message
 * @return The transport or dispatch status.
 */
v_status_t vti_call(vti_desc_t *desc, vii_msg_t *msg)
{
    return vedam_node_call(desc, msg);
}

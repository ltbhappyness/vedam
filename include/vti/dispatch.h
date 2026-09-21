/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VTI_DISPATCH_H
#define VTI_DISPATCH_H

#include "vii/types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vedam_node;

/**
 * @brief One vii verb entry point.
 *
 * @param[in]     origin Call origin
 * @param[in]     major  Vertical major
 * @param[in]     minor  Vertical minor
 * @param[in,out] msg    Public message
 * @return The verb status.
 */
typedef v_status_t (*vta_verb_fn)(v_origin_t origin, vuint32_t major, vuint32_t minor,
                                  vi_msg_t *msg);

/**
 * @brief Seven verb slots for one class id.
 */
struct vta_class_verbs {
    vta_verb_fn fn[7];
};

/**
 * @brief Fill the per-class verb table on @p node.
 *
 * @param[in,out] node Process node that owns the table
 * @return V_STATUS_SUCCESS, or V_STATUS_NO_MEMORY when the table cannot be allocated.
 */
v_status_t vta_dispatch_table_init(struct vedam_node *node);

/**
 * @brief Free the verb table on @p node.
 *
 * @param[in,out] node Process node that owns the table
 */
void vta_dispatch_table_fini(struct vedam_node *node);

/**
 * @brief Call the vii verb for @p call through the process lock map.
 *
 * @param[in,out] call Transport message (class, verb, origin, version, and vi_msg_t)
 * @return The vii status, or V_STATUS_NOT_IMPLEMENTED when the class or verb is unknown.
 */
v_status_t vta_dispatch(vta_msg_t *call);

#ifdef __cplusplus
}
#endif

#endif

/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VLIB_PLUGIN_H
#define VLIB_PLUGIN_H

#include "vi/types.h"

/**
 * @brief Bring up a plugin object from an internal attribute list.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message (key and attributes)
 * @return V_STATUS_NOT_IMPLEMENTED, or V_STATUS_NOT_SUPPORTED when plugin is left out.
 */
v_status_t vlib_plugin_create(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

/**
 * @brief Fill values for the attribute ids in the message.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED, or V_STATUS_NOT_SUPPORTED when plugin is left out.
 */
v_status_t vlib_plugin_get(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

/**
 * @brief Change attributes that allow set.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED, or V_STATUS_NOT_SUPPORTED when plugin is left out.
 */
v_status_t vlib_plugin_set(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

/**
 * @brief Tear down a plugin object.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message (key only)
 * @return V_STATUS_NOT_IMPLEMENTED, or V_STATUS_NOT_SUPPORTED when plugin is left out.
 */
v_status_t vlib_plugin_delete(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

/**
 * @brief Watch SUBSCRIBABLE attributes.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED, or V_STATUS_NOT_SUPPORTED when plugin is left out.
 */
v_status_t vlib_plugin_subscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

/**
 * @brief Stop watches for the same attribute id list.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED, or V_STATUS_NOT_SUPPORTED when plugin is left out.
 */
v_status_t vlib_plugin_unsubscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

/**
 * @brief Push changed attributes to current subscribers.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED, or V_STATUS_NOT_SUPPORTED when plugin is left out.
 */
v_status_t vlib_plugin_notify(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

#endif

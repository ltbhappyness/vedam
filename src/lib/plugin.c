/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "lib/plugin.h"

#if VEDAM_ENABLE_PLUGIN

/**
 * @brief Report that no program is loaded.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
static v_status_t plugin_unimplemented(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Bring up a plugin object from an internal attribute list.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
v_status_t vlib_plugin_create(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unimplemented(major, minor, msg);
}

/**
 * @brief Fill values for the attribute ids in the message.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
v_status_t vlib_plugin_get(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unimplemented(major, minor, msg);
}

/**
 * @brief Change attributes that allow set.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
v_status_t vlib_plugin_set(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unimplemented(major, minor, msg);
}

/**
 * @brief Tear down a plugin object.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
v_status_t vlib_plugin_delete(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unimplemented(major, minor, msg);
}

/**
 * @brief Watch SUBSCRIBABLE attributes.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
v_status_t vlib_plugin_subscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unimplemented(major, minor, msg);
}

/**
 * @brief Stop watches for the same attribute id list.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
v_status_t vlib_plugin_unsubscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unimplemented(major, minor, msg);
}

/**
 * @brief Push changed attributes to current subscribers.
 *
 * The call does not load a program.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
v_status_t vlib_plugin_notify(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unimplemented(major, minor, msg);
}

#else

/**
 * @brief Report that plugin is left out of this build.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
static v_status_t plugin_unsupported(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Bring up a plugin object from an internal attribute list.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_plugin_create(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unsupported(major, minor, msg);
}

/**
 * @brief Fill values for the attribute ids in the message.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_plugin_get(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unsupported(major, minor, msg);
}

/**
 * @brief Change attributes that allow set.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_plugin_set(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unsupported(major, minor, msg);
}

/**
 * @brief Tear down a plugin object.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_plugin_delete(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unsupported(major, minor, msg);
}

/**
 * @brief Watch SUBSCRIBABLE attributes.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_plugin_subscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unsupported(major, minor, msg);
}

/**
 * @brief Stop watches for the same attribute id list.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_plugin_unsubscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unsupported(major, minor, msg);
}

/**
 * @brief Push changed attributes to current subscribers.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_plugin_notify(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return plugin_unsupported(major, minor, msg);
}

#endif /* VEDAM_ENABLE_PLUGIN */

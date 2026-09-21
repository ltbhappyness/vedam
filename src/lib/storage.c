/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "lib/storage.h"

/**
 * @brief Bring up a storage object from an internal attribute list.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS.
 */
v_status_t vlib_storage_create(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Fill values for the attribute ids in the message.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS.
 */
v_status_t vlib_storage_get(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Change attributes that allow set.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS.
 */
v_status_t vlib_storage_set(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Tear down a storage object.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS.
 */
v_status_t vlib_storage_delete(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Watch SUBSCRIBABLE attributes.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS.
 */
v_status_t vlib_storage_subscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Stop watches for the same attribute id list.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS.
 */
v_status_t vlib_storage_unsubscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Push changed attributes to current subscribers.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS.
 */
v_status_t vlib_storage_notify(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_SUCCESS;
}

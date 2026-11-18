/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "lib/contract.h"

#if VEDAM_ENABLE_CONTRACT

/**
 * @brief Bring up a contract object from an internal attribute list.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS.
 */
v_status_t vlib_contract_create(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Fill values for the attribute ids in the message.
 *
 * The call does not run a program and does not write a value.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED.
 */
v_status_t vlib_contract_get(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Change attributes that allow set.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS.
 */
v_status_t vlib_contract_set(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Tear down a contract object.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS.
 */
v_status_t vlib_contract_delete(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
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
v_status_t vlib_contract_subscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
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
v_status_t vlib_contract_unsubscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
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
v_status_t vlib_contract_notify(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_SUCCESS;
}

#else

/**
 * @brief Report that contract is left out of this build.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
static v_status_t contract_unsupported(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    (void)major;
    (void)minor;
    (void)msg;
    return V_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Bring up a contract object from an internal attribute list.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_contract_create(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return contract_unsupported(major, minor, msg);
}

/**
 * @brief Fill values for the attribute ids in the message.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_contract_get(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return contract_unsupported(major, minor, msg);
}

/**
 * @brief Change attributes that allow set.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_contract_set(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return contract_unsupported(major, minor, msg);
}

/**
 * @brief Tear down a contract object.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_contract_delete(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return contract_unsupported(major, minor, msg);
}

/**
 * @brief Watch SUBSCRIBABLE attributes.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_contract_subscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return contract_unsupported(major, minor, msg);
}

/**
 * @brief Stop watches for the same attribute id list.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_contract_unsubscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return contract_unsupported(major, minor, msg);
}

/**
 * @brief Push changed attributes to current subscribers.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_SUPPORTED.
 */
v_status_t vlib_contract_notify(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    return contract_unsupported(major, minor, msg);
}

#endif /* VEDAM_ENABLE_CONTRACT */

/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VLIB_CONTRACT_H
#define VLIB_CONTRACT_H

#include "vi/types.h"

/**
 * @brief Bring up a contract object from an internal attribute list.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message (key and attributes)
 * @return V_STATUS_SUCCESS, or V_STATUS_NOT_SUPPORTED when contract is left out.
 */
v_status_t vlib_contract_create(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

/**
 * @brief Fill values for the attribute ids in the message.
 *
 * The call does not run a program and does not write a value.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_NOT_IMPLEMENTED, or V_STATUS_NOT_SUPPORTED when contract is left out.
 */
v_status_t vlib_contract_get(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

/**
 * @brief Change attributes that allow set.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS, or V_STATUS_NOT_SUPPORTED when contract is left out.
 */
v_status_t vlib_contract_set(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

/**
 * @brief Tear down a contract object.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message (key only)
 * @return V_STATUS_SUCCESS, or V_STATUS_NOT_SUPPORTED when contract is left out.
 */
v_status_t vlib_contract_delete(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

/**
 * @brief Watch SUBSCRIBABLE attributes.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS, or V_STATUS_NOT_SUPPORTED when contract is left out.
 */
v_status_t vlib_contract_subscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

/**
 * @brief Stop watches for the same attribute id list.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS, or V_STATUS_NOT_SUPPORTED when contract is left out.
 */
v_status_t vlib_contract_unsubscribe(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

/**
 * @brief Push changed attributes to current subscribers.
 *
 * @param[in]     major Vertical major from the UUID line
 * @param[in]     minor Vertical minor from the UUID line
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS, or V_STATUS_NOT_SUPPORTED when contract is left out.
 */
v_status_t vlib_contract_notify(vuint32_t major, vuint32_t minor, vi_msg_t *msg);

#endif

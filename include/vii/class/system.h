/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#ifndef VII_CLASS_SYSTEM_H
#define VII_CLASS_SYSTEM_H

#include "vi/class/system.h"
#include "vii/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Highest major index for a supported system UUID line.
 */
enum { VII_SYSTEM_UUID_MAJOR_MAX = 1 };

/**
 * @brief Highest minor index for a supported system UUID line.
 */
enum { VII_SYSTEM_UUID_MINOR_MAX = 0 };

/**
 * @brief Latest major for local system calls.
 */
enum { VII_SYSTEM_MAJOR = 1 };

/**
 * @brief Latest minor for local system calls.
 */
enum { VII_SYSTEM_MINOR = 0 };

/**
 * @brief Every supported system UUID line, indexed by major then minor.
 *
 * A slot with no YAML line is left zero. The public header keeps only the latest
 * line; this table is for the library and the edge map.
 */
static const uint8_t vii_system_uuid[VII_SYSTEM_UUID_MAJOR_MAX + 1]
                                            [VII_SYSTEM_UUID_MINOR_MAX + 1]
                                            [16] = {
    {
        { 0 },
    },
    {
        { 0xc2, 0xb9, 0x15, 0x80, 0x60, 0x92, 0x5c, 0x64, 0xbd, 0x0c, 0xea, 0x7f, 0x37, 0x7b, 0x56, 0xc1 },
    },
};

/**
 * @brief Internal verbs for system. Attribute ids live in vi/class/system.h.
 */

/**
 * @brief Create the system object. Origin must be internal.
 *
 * @param[in]     origin Call origin
 * @param[in]     major  Vertical major from the UUID line
 * @param[in]     minor  Vertical minor from the UUID line
 * @param[in,out] msg    Call message (key and attributes)
 * @return The library status, or V_STATUS_NOT_IMPLEMENTED for an external origin.
 */
v_status_t vii_system_create(v_origin_t origin, vuint32_t major, vuint32_t minor,
                                       vi_msg_t *msg);

/**
 * @brief Fill attribute values after contract checks.
 *
 * @param[in]     origin Call origin
 * @param[in]     major  Vertical major from the UUID line
 * @param[in]     minor  Vertical minor from the UUID line
 * @param[in,out] msg    Call message
 * @return The library status after the contract checks.
 */
v_status_t vii_system_get(v_origin_t origin, vuint32_t major, vuint32_t minor,
                                    vi_msg_t *msg);

/**
 * @brief Change attributes that allow set. Origin must be internal.
 *
 * @param[in]     origin Call origin
 * @param[in]     major  Vertical major from the UUID line
 * @param[in]     minor  Vertical minor from the UUID line
 * @param[in,out] msg    Call message
 * @return The library status, or V_STATUS_NOT_IMPLEMENTED for an external origin.
 */
v_status_t vii_system_set(v_origin_t origin, vuint32_t major, vuint32_t minor,
                                    vi_msg_t *msg);

/**
 * @brief Tear down the system object. Origin must be internal.
 *
 * @param[in]     origin Call origin
 * @param[in]     major  Vertical major from the UUID line
 * @param[in]     minor  Vertical minor from the UUID line
 * @param[in,out] msg    Call message (key only)
 * @return The library status, or V_STATUS_NOT_IMPLEMENTED for an external origin.
 */
v_status_t vii_system_delete(v_origin_t origin, vuint32_t major, vuint32_t minor,
                                       vi_msg_t *msg);

/**
 * @brief Watch SUBSCRIBABLE attributes.
 *
 * @param[in]     origin Call origin
 * @param[in]     major  Vertical major from the UUID line
 * @param[in]     minor  Vertical minor from the UUID line
 * @param[in,out] msg    Call message
 * @return The library status after the contract checks.
 */
v_status_t vii_system_subscribe(v_origin_t origin, vuint32_t major, vuint32_t minor,
                                          vi_msg_t *msg);

/**
 * @brief Stop watches for the same attribute id list.
 *
 * @param[in]     origin Call origin
 * @param[in]     major  Vertical major from the UUID line
 * @param[in]     minor  Vertical minor from the UUID line
 * @param[in,out] msg    Call message
 * @return The library status.
 */
v_status_t vii_system_unsubscribe(v_origin_t origin, vuint32_t major, vuint32_t minor,
                                            vi_msg_t *msg);

/**
 * @brief Push changed attributes to current subscribers.
 *
 * @param[in]     origin Call origin
 * @param[in]     major  Vertical major from the UUID line
 * @param[in]     minor  Vertical minor from the UUID line
 * @param[in,out] msg    Call message
 * @return The library status, or V_STATUS_NOT_IMPLEMENTED for an external origin.
 */
v_status_t vii_system_notify(v_origin_t origin, vuint32_t major, vuint32_t minor,
                                       vi_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* VII_CLASS_SYSTEM_H */

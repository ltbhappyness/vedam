/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#ifndef VII_CLASS_WASM_H
#define VII_CLASS_WASM_H

#include "vi/class/wasm.h"
#include "vii/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Highest major index for a supported wasm UUID line.
 */
enum { VII_WASM_UUID_MAJOR_MAX = 1 };

/**
 * @brief Highest minor index for a supported wasm UUID line.
 */
enum { VII_WASM_UUID_MINOR_MAX = 0 };

/**
 * @brief Latest major for local wasm calls.
 */
enum { VII_WASM_MAJOR = 1 };

/**
 * @brief Latest minor for local wasm calls.
 */
enum { VII_WASM_MINOR = 0 };

/**
 * @brief Every supported wasm UUID line, indexed by major then minor.
 *
 * A slot with no YAML line is left zero. The public header keeps only the latest
 * line; this table is for the library and the edge map.
 */
static const uint8_t vii_wasm_uuid[VII_WASM_UUID_MAJOR_MAX + 1]
                                            [VII_WASM_UUID_MINOR_MAX + 1]
                                            [16] = {
    {
        { 0 },
    },
    {
        { 0x05, 0x3e, 0xf9, 0xba, 0x73, 0x54, 0x58, 0x91, 0xad, 0x47, 0x7a, 0x52, 0x9f, 0x4c, 0x1b, 0x64 },
    },
};

/**
 * @brief Internal verbs for wasm. Attribute ids live in vi/class/wasm.h.
 */

/**
 * @brief Create the wasm object. Origin must be internal.
 *
 * @param[in]     origin Call origin
 * @param[in]     major  Vertical major from the UUID line
 * @param[in]     minor  Vertical minor from the UUID line
 * @param[in,out] msg    Call message (key and attributes)
 * @return The library status, or V_STATUS_NOT_IMPLEMENTED for an external origin.
 */
v_status_t vii_wasm_create(v_origin_t origin, vuint32_t major, vuint32_t minor,
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
v_status_t vii_wasm_get(v_origin_t origin, vuint32_t major, vuint32_t minor,
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
v_status_t vii_wasm_set(v_origin_t origin, vuint32_t major, vuint32_t minor,
                                    vi_msg_t *msg);

/**
 * @brief Tear down the wasm object. Origin must be internal.
 *
 * @param[in]     origin Call origin
 * @param[in]     major  Vertical major from the UUID line
 * @param[in]     minor  Vertical minor from the UUID line
 * @param[in,out] msg    Call message (key only)
 * @return The library status, or V_STATUS_NOT_IMPLEMENTED for an external origin.
 */
v_status_t vii_wasm_delete(v_origin_t origin, vuint32_t major, vuint32_t minor,
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
v_status_t vii_wasm_subscribe(v_origin_t origin, vuint32_t major, vuint32_t minor,
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
v_status_t vii_wasm_unsubscribe(v_origin_t origin, vuint32_t major, vuint32_t minor,
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
v_status_t vii_wasm_notify(v_origin_t origin, vuint32_t major, vuint32_t minor,
                                       vi_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* VII_CLASS_WASM_H */

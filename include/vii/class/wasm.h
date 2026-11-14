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
 * @brief Every wasm attribute identifier, public and internal.
 *
 * A public id has the same number as in vi/class/wasm.h.
 */
typedef enum _vii_wasm_attr_t {
    VII_WASM_ATTR_UNSPECIFIED = 0,
    /**
     * @brief Configuration state of the WebAssembly engine slot.
     * @type vint32_t
     * @kind S32
     * @flags READ_ONLY | SUBSCRIBABLE
     */
    VII_WASM_ATTR_ENGINE_STATE = 1,
    /**
     * @brief Configured worker count for the WebAssembly slot.
     * @type vint32_t
     * @kind S32
     * @flags READ_ONLY
     */
    VII_WASM_ATTR_WORKERS = 2,
} vii_wasm_attr_t;

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
static const uint8_t vii_wasm_uuid[][VII_WASM_UUID_MINOR_MAX + 1][16] = {
    {
        {0},
    },
    {
        {
            0x05,
            0x3e,
            0xf9,
            0xba,
            0x73,
            0x54,
            0x58,
            0x91,
            0xad,
            0x47,
            0x7a,
            0x52,
            0x9f,
            0x4c,
            0x1b,
            0x64,
        },
    },
};

#ifdef __cplusplus
}
#endif

#endif /* VII_CLASS_WASM_H */

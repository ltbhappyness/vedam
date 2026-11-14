/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#ifndef VII_CLASS_STORAGE_H
#define VII_CLASS_STORAGE_H

#include "vi/class/storage.h"
#include "vii/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Every storage attribute identifier, public and internal.
 *
 * A public id has the same number as in vi/class/storage.h.
 */
typedef enum _vii_storage_attr_t {
    VII_STORAGE_ATTR_UNSPECIFIED = 0,
    /**
     * @brief Size of this storage object.
     * @type vint32_t
     * @kind S32
     * @flags READ_ONLY
     */
    VII_STORAGE_ATTR_SIZE = 1,
    /**
     * @brief State of this storage object.
     * @type vint32_t
     * @kind S32
     * @flags READ_ONLY | SUBSCRIBABLE
     */
    VII_STORAGE_ATTR_STATE = 2,
} vii_storage_attr_t;

/**
 * @brief Highest major index for a supported storage UUID line.
 */
enum { VII_STORAGE_UUID_MAJOR_MAX = 1 };

/**
 * @brief Highest minor index for a supported storage UUID line.
 */
enum { VII_STORAGE_UUID_MINOR_MAX = 0 };

/**
 * @brief Latest major for local storage calls.
 */
enum { VII_STORAGE_MAJOR = 1 };

/**
 * @brief Latest minor for local storage calls.
 */
enum { VII_STORAGE_MINOR = 0 };

/**
 * @brief Every supported storage UUID line, indexed by major then minor.
 *
 * A slot with no YAML line is left zero. The public header keeps only the latest
 * line; this table is for the library and the edge map.
 */
static const uint8_t vii_storage_uuid[][VII_STORAGE_UUID_MINOR_MAX + 1][16] = {
    {
        {0},
    },
    {
        {
            0x77,
            0x14,
            0x5e,
            0x11,
            0xa4,
            0x7c,
            0x50,
            0xda,
            0xbc,
            0x42,
            0xa9,
            0x1a,
            0xd8,
            0xd7,
            0x64,
            0xb8,
        },
    },
};

#ifdef __cplusplus
}
#endif

#endif /* VII_CLASS_STORAGE_H */

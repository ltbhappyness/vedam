/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#ifndef VII_CLASS_IDENTITY_H
#define VII_CLASS_IDENTITY_H

#include "vi/class/identity.h"
#include "vii/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Every identity attribute identifier, public and internal.
 *
 * A public id has the same number as in vi/class/identity.h.
 */
typedef enum _vii_identity_attr_t {
    VII_IDENTITY_ATTR_UNSPECIFIED = 0,
    /**
     * @brief Kind of this identity object.
     * @type vint32_t
     * @kind S32
     * @flags READ_ONLY
     */
    VII_IDENTITY_ATTR_KIND = 1,
    /**
     * @brief State of this identity object.
     * @type vint32_t
     * @kind S32
     * @flags READ_ONLY | SUBSCRIBABLE
     */
    VII_IDENTITY_ATTR_STATE = 2,
} vii_identity_attr_t;

/**
 * @brief Highest major index for a supported identity UUID line.
 */
enum { VII_IDENTITY_UUID_MAJOR_MAX = 1 };

/**
 * @brief Highest minor index for a supported identity UUID line.
 */
enum { VII_IDENTITY_UUID_MINOR_MAX = 0 };

/**
 * @brief Latest major for local identity calls.
 */
enum { VII_IDENTITY_MAJOR = 1 };

/**
 * @brief Latest minor for local identity calls.
 */
enum { VII_IDENTITY_MINOR = 0 };

/**
 * @brief Every supported identity UUID line, indexed by major then minor.
 *
 * A slot with no YAML line is left zero. The public header keeps only the latest
 * line; this table is for the library and the edge map.
 */
static const uint8_t vii_identity_uuid[][VII_IDENTITY_UUID_MINOR_MAX + 1][16] = {
    {
        {0},
    },
    {
        {
            0x29,
            0xeb,
            0x31,
            0x2d,
            0x05,
            0x7d,
            0x59,
            0x30,
            0xbc,
            0x70,
            0xbc,
            0x6b,
            0x01,
            0xcb,
            0x45,
            0xab,
        },
    },
};

#ifdef __cplusplus
}
#endif

#endif /* VII_CLASS_IDENTITY_H */

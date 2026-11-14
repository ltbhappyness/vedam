/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#ifndef VII_CLASS_SECURITY_H
#define VII_CLASS_SECURITY_H

#include "vi/class/security.h"
#include "vii/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Every security attribute identifier, public and internal.
 *
 * A public id has the same number as in vi/class/security.h.
 */
typedef enum _vii_security_attr_t {
    VII_SECURITY_ATTR_UNSPECIFIED = 0,
    /**
     * @brief Active security profile for this process.
     * @type vint32_t
     * @kind S32
     * @flags READ_ONLY
     */
    VII_SECURITY_ATTR_PROFILE = 1,
    /**
     * @brief State of the security singleton.
     * @type vint32_t
     * @kind S32
     * @flags READ_ONLY | SUBSCRIBABLE
     */
    VII_SECURITY_ATTR_STATE = 2,
} vii_security_attr_t;

/**
 * @brief Highest major index for a supported security UUID line.
 */
enum { VII_SECURITY_UUID_MAJOR_MAX = 1 };

/**
 * @brief Highest minor index for a supported security UUID line.
 */
enum { VII_SECURITY_UUID_MINOR_MAX = 0 };

/**
 * @brief Latest major for local security calls.
 */
enum { VII_SECURITY_MAJOR = 1 };

/**
 * @brief Latest minor for local security calls.
 */
enum { VII_SECURITY_MINOR = 0 };

/**
 * @brief Every supported security UUID line, indexed by major then minor.
 *
 * A slot with no YAML line is left zero. The public header keeps only the latest
 * line; this table is for the library and the edge map.
 */
static const uint8_t vii_security_uuid[][VII_SECURITY_UUID_MINOR_MAX + 1][16] = {
    {
        {0},
    },
    {
        {
            0x3d,
            0x8d,
            0x56,
            0x23,
            0x8c,
            0xa8,
            0x58,
            0x5f,
            0xa4,
            0x34,
            0x66,
            0x4c,
            0x1d,
            0xf3,
            0x8f,
            0xfa,
        },
    },
};

#ifdef __cplusplus
}
#endif

#endif /* VII_CLASS_SECURITY_H */

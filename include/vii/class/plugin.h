/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#ifndef VII_CLASS_PLUGIN_H
#define VII_CLASS_PLUGIN_H

#include "vi/class/plugin.h"
#include "vii/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Every plugin attribute identifier, public and internal.
 *
 * A public id has the same number as in vi/class/plugin.h.
 */
typedef enum _vii_plugin_attr_t {
    VII_PLUGIN_ATTR_UNSPECIFIED = 0,
    /**
     * @brief Local program record. Other nodes do not replay it.
     * @type vint32_t
     * @kind S32
     * @flags READ_ONLY
     */
    VII_PLUGIN_ATTR_STATE = 1,
} vii_plugin_attr_t;

/**
 * @brief Highest major index for a supported plugin UUID line.
 */
enum { VII_PLUGIN_UUID_MAJOR_MAX = 1 };

/**
 * @brief Highest minor index for a supported plugin UUID line.
 */
enum { VII_PLUGIN_UUID_MINOR_MAX = 0 };

/**
 * @brief Latest major for local plugin calls.
 */
enum { VII_PLUGIN_MAJOR = 1 };

/**
 * @brief Latest minor for local plugin calls.
 */
enum { VII_PLUGIN_MINOR = 0 };

/**
 * @brief Every supported plugin UUID line, indexed by major then minor.
 *
 * A slot with no YAML line is left zero. The public header keeps only the latest
 * line; this table is for the library and the edge map.
 */
static const uint8_t vii_plugin_uuid[][VII_PLUGIN_UUID_MINOR_MAX + 1][16] = {
    {
        {0},
    },
    {
        {
            0xc8,
            0xb1,
            0xba,
            0x03,
            0xa6,
            0x73,
            0x5b,
            0xa4,
            0x95,
            0x04,
            0x45,
            0x23,
            0xcf,
            0xaf,
            0xc7,
            0x62,
        },
    },
};

#ifdef __cplusplus
}
#endif

#endif /* VII_CLASS_PLUGIN_H */

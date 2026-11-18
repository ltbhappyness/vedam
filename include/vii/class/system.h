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
 * @brief Every system attribute identifier, public and internal.
 *
 * A public id has the same number as in vi/class/system.h.
 */
typedef enum _vii_system_attr_t {
    VII_SYSTEM_ATTR_UNSPECIFIED = 0,
    /**
     * @brief The letter this build was compiled as. Not a runtime job title.
     * @type vi_system_node_class_t
     * @kind S32
     * @values vi_system_node_class_t
     * @flags READ_ONLY
     */
    VII_SYSTEM_ATTR_NODE_CLASS = 1,
    /**
     * @brief Agreed message version for this node. This era always reports major 0 and minor 1.
     * @type v_version_t
     * @kind VERSION
     * @flags READ_ONLY | SUBSCRIBABLE
     */
    VII_SYSTEM_ATTR_PROTOCOL_VERSION = 2,
    /**
     * @brief Whether the process is starting, ready, degraded, draining, or dead. Pass value.ptr to a caller-owned vi_system_state_t.
     * @type vi_system_state_t
     * @kind STRUCT
     * @flags READ_ONLY | SUBSCRIBABLE
     */
    VII_SYSTEM_ATTR_SYSTEM_STATE = 3,
    /**
     * @brief Commit of the running build, for a person to read. Not an input to the vertical UUID.
     * @type v_text_t
     * @kind TEXT
     * @get Set len to the room in buf. On overflow len holds the needed length.
     * @flags READ_ONLY
     */
    VII_SYSTEM_ATTR_RELEASE_COMMIT = 4,
} vii_system_attr_t;

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
static const uint8_t vii_system_uuid[][VII_SYSTEM_UUID_MINOR_MAX + 1][16] = {
    {
        {0},
    },
    {
        {
            0xc2,
            0xb9,
            0x15,
            0x80,
            0x60,
            0x92,
            0x5c,
            0x64,
            0xbd,
            0x0c,
            0xea,
            0x7f,
            0x37,
            0x7b,
            0x56,
            0xc1,
        },
    },
};

#ifdef __cplusplus
}
#endif

#endif /* VII_CLASS_SYSTEM_H */

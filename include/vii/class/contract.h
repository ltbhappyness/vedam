/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#ifndef VII_CLASS_CONTRACT_H
#define VII_CLASS_CONTRACT_H

#include "vi/class/contract.h"
#include "vii/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Every contract attribute identifier, public and internal.
 *
 * A public id has the same number as in vi/class/contract.h.
 */
typedef enum _vii_contract_attr_t {
    VII_CONTRACT_ATTR_UNSPECIFIED = 0,
    /**
     * @brief Byte ceiling for the contract program. It is not part of the digest.
     * @type vuint32_t
     * @kind U32
     * @flags READ_ONLY
     */
    VII_CONTRACT_ATTR_MEMORY_LIMIT = 3,
    /**
     * @brief Instruction ceiling for one run. A run that reaches it does not commit.
     * @type vuint32_t
     * @kind U32
     * @flags READ_ONLY
     */
    VII_CONTRACT_ATTR_BUDGET = 4,
} vii_contract_attr_t;

/**
 * @brief Highest major index for a supported contract UUID line.
 */
enum { VII_CONTRACT_UUID_MAJOR_MAX = 1 };

/**
 * @brief Highest minor index for a supported contract UUID line.
 */
enum { VII_CONTRACT_UUID_MINOR_MAX = 0 };

/**
 * @brief Latest major for local contract calls.
 */
enum { VII_CONTRACT_MAJOR = 1 };

/**
 * @brief Latest minor for local contract calls.
 */
enum { VII_CONTRACT_MINOR = 0 };

/**
 * @brief Every supported contract UUID line, indexed by major then minor.
 *
 * A slot with no YAML line is left zero. The public header keeps only the latest
 * line; this table is for the library and the edge map.
 */
static const uint8_t vii_contract_uuid[][VII_CONTRACT_UUID_MINOR_MAX + 1][16] = {
    {
        {0},
    },
    {
        {
            0xf8,
            0xc5,
            0xc5,
            0x5f,
            0x54,
            0xb9,
            0x5e,
            0x5b,
            0xad,
            0xea,
            0xd3,
            0x92,
            0xc8,
            0xb4,
            0x8c,
            0xcd,
        },
    },
};

#ifdef __cplusplus
}
#endif

#endif /* VII_CLASS_CONTRACT_H */

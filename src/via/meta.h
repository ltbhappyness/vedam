/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#ifndef VIA_META_H
#define VIA_META_H

#include "vi/types.h"

/**
 * @brief Attribute contract flags used by VIA checks.
 */
enum {
    VIA_F_MANDATORY_ON_CREATE = 1u << 0,
    VIA_F_CREATE_ONLY = 1u << 1,
    VIA_F_CREATE_AND_SET = 1u << 2,
    VIA_F_READ_ONLY = 1u << 3,
    VIA_F_SET_ONLY = 1u << 4,
    VIA_F_KEY = 1u << 5,
    VIA_F_SUBSCRIBABLE = 1u << 6
};

/**
 * @brief Attribute value kinds for VIA type checks.
 */
enum {
    VIA_T_NONE = 0,
    VIA_T_S32 = 1,
    VIA_T_VERSION = 2,
    VIA_T_PTR = 3,
    VIA_T_BOOL = 4,
    VIA_T_TEXT = 5,
    VIA_T_TRANSPORT = 6
};

/**
 * @brief One row of attribute metadata for a class.
 */
struct via_attr_meta {
    v_attr_id_t id;
    int is_public;
    unsigned flags;
    int type_kind;
};

/**
 * @brief Find metadata for @p id in @p tab, or NULL if unknown.
 */
const struct via_attr_meta *via_attr_find(const struct via_attr_meta *tab, vuint32_t n,
                                          v_attr_id_t id);

#endif /* VIA_META_H */

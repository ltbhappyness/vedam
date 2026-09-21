/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VTI_ATTRWIRE_H
#define VTI_ATTRWIRE_H

#include "vii/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief YAML type kind for @p id, or -1 when the class does not define it.
 */
int vta_attr_kind(v_class_id_t class_id, v_attr_id_t id);

/**
 * @brief Byte size of a pointer-typed attribute, or 0 when it has none.
 */
vuint32_t vta_attr_ptr_size(v_class_id_t class_id, v_attr_id_t id);

#ifdef __cplusplus
}
#endif

#endif

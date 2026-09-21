/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#include "vii/types.h"

/**
 * @brief Non-zero when @p id is an attribute the class YAML defines.
 *
 * @param[in] class_id Local class index
 * @param[in] id       Attribute id
 * @return 1 when known, 0 when not.
 */
int vedam_lock_attr_known(v_class_id_t class_id, v_attr_id_t id)
{
    switch (class_id) {
    case V_CLASS_IDENTITY:
        switch (id) {
        case 1:
        case 2:
            return 1;
        default:
            return 0;
        }
    case V_CLASS_SECURITY:
        switch (id) {
        case 1:
        case 2:
            return 1;
        default:
            return 0;
        }
    case V_CLASS_STORAGE:
        switch (id) {
        case 1:
        case 2:
            return 1;
        default:
            return 0;
        }
    case V_CLASS_SYSTEM:
        switch (id) {
        case 1:
        case 2:
        case 3:
        case 4:
            return 1;
        default:
            return 0;
        }
    case V_CLASS_WASM:
        switch (id) {
        case 1:
        case 2:
            return 1;
        default:
            return 0;
        }
    default:
        return 0;
    }
}

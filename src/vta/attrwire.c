/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#include "vti/attrwire.h"

#include "meta.h"
#include "vi/class/identity.h"
#include "vi/class/security.h"
#include "vi/class/storage.h"
#include "vi/class/system.h"
#include "vi/class/wasm.h"

/**
 * @brief Return the VIA type kind for @p id on @p class_id.
 *
 * @param[in] class_id Local class index
 * @param[in] id       Attribute id
 * @return A VIA_T_* kind, or -1 when unknown.
 */
int vta_attr_kind(v_class_id_t class_id, v_attr_id_t id)
{
    switch (class_id) {
    case V_CLASS_IDENTITY:
        switch (id) {
        case 1:
            return VIA_T_S32;
        case 2:
            return VIA_T_S32;
        default:
            return -1;
        }
    case V_CLASS_SECURITY:
        switch (id) {
        case 1:
            return VIA_T_S32;
        case 2:
            return VIA_T_S32;
        default:
            return -1;
        }
    case V_CLASS_STORAGE:
        switch (id) {
        case 1:
            return VIA_T_S32;
        case 2:
            return VIA_T_S32;
        default:
            return -1;
        }
    case V_CLASS_SYSTEM:
        switch (id) {
        case 1:
            return VIA_T_S32;
        case 2:
            return VIA_T_VERSION;
        case 3:
            return VIA_T_PTR;
        case 4:
            return VIA_T_TEXT;
        default:
            return -1;
        }
    case V_CLASS_WASM:
        switch (id) {
        case 1:
            return VIA_T_S32;
        case 2:
            return VIA_T_S32;
        default:
            return -1;
        }
    default:
        return -1;
    }
}

/**
 * @brief Byte size of a pointer-valued attribute for the queue codec.
 *
 * @param[in] class_id Local class index
 * @param[in] id       Attribute id
 * @return Size in bytes, or 0 when the attribute is not a pointer kind.
 */
vuint32_t vta_attr_ptr_size(v_class_id_t class_id, v_attr_id_t id)
{
    if (class_id == V_CLASS_IDENTITY) {
        switch (id) {
        default:
            return 0;
        }
    }
    if (class_id == V_CLASS_SECURITY) {
        switch (id) {
        default:
            return 0;
        }
    }
    if (class_id == V_CLASS_STORAGE) {
        switch (id) {
        default:
            return 0;
        }
    }
    if (class_id == V_CLASS_SYSTEM) {
        switch (id) {
        case 3:
            return (vuint32_t)sizeof(vi_system_state_t);
        default:
            return 0;
        }
    }
    if (class_id == V_CLASS_WASM) {
        switch (id) {
        default:
            return 0;
        }
    }
    (void)id;
    return 0;
}

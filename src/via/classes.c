/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#include "lib/contract.h"
#include "lib/identity.h"
#include "lib/plugin.h"
#include "lib/security.h"
#include "lib/storage.h"
#include "lib/system.h"
#include "vii/class/contract.h"
#include "vii/class/identity.h"
#include "vii/class/plugin.h"
#include "vii/class/security.h"
#include "vii/class/storage.h"
#include "vii/class/system.h"
#include "vii/meta.h"

const struct vii_verb_rule vii_verb_rules[V_VERB_COUNT] = {
    [V_VERB_CREATE] = {.collapse = 0, .checks_list = 1, .writes_values = 1},
    [V_VERB_GET] = {.collapse = 1, .checks_list = 1, .writes_values = 0},
    [V_VERB_SET] = {.collapse = 1, .checks_list = 1, .writes_values = 1},
    [V_VERB_DELETE] = {.collapse = 0, .checks_list = 0, .writes_values = 0},
    [V_VERB_SUBSCRIBE] = {.collapse = 1, .checks_list = 1, .writes_values = 0},
    [V_VERB_UNSUBSCRIBE] = {.collapse = 1, .checks_list = 1, .writes_values = 0},
    [V_VERB_NOTIFY] = {.collapse = 0, .checks_list = 1, .writes_values = 1},
};

static const struct vii_attr_meta contract_attrs[] = {
    {
        .id = 3,
        .flags = VII_F_READ_ONLY,
        .kind = VII_T_U32,
        .is_public = 0,
    },
    {
        .id = 4,
        .flags = VII_F_READ_ONLY,
        .kind = VII_T_U32,
        .is_public = 0,
    },
};

static const vuint8_t contract_attr_by_id[] = {0, 0, 0, 1, 2};

static const struct vii_attr_meta identity_attrs[] = {
    {
        .id = 1,
        .flags = VII_F_READ_ONLY,
        .kind = VII_T_S32,
        .is_public = 1,
    },
    {
        .id = 2,
        .flags = VII_F_READ_ONLY | VII_F_SUBSCRIBABLE,
        .kind = VII_T_S32,
        .is_public = 1,
    },
};

static const vuint8_t identity_attr_by_id[] = {0, 1, 2};

static const struct vii_attr_meta plugin_attrs[] = {
    {
        .id = 1,
        .flags = VII_F_READ_ONLY,
        .kind = VII_T_S32,
        .is_public = 0,
    },
};

static const vuint8_t plugin_attr_by_id[] = {0, 1};

static const struct vii_attr_meta security_attrs[] = {
    {
        .id = 1,
        .flags = VII_F_READ_ONLY,
        .kind = VII_T_S32,
        .is_public = 1,
    },
    {
        .id = 2,
        .flags = VII_F_READ_ONLY | VII_F_SUBSCRIBABLE,
        .kind = VII_T_S32,
        .is_public = 1,
    },
};

static const vuint8_t security_attr_by_id[] = {0, 1, 2};

static const struct vii_attr_meta storage_attrs[] = {
    {
        .id = 1,
        .flags = VII_F_READ_ONLY,
        .kind = VII_T_S32,
        .is_public = 1,
    },
    {
        .id = 2,
        .flags = VII_F_READ_ONLY | VII_F_SUBSCRIBABLE,
        .kind = VII_T_S32,
        .is_public = 1,
    },
};

static const vuint8_t storage_attr_by_id[] = {0, 1, 2};

static const vint32_t vi_system_node_class_t_values[] = {0, 1, 2, 3, 4, 5};

static const struct vii_attr_meta system_attrs[] = {
    {
        .id = 1,
        .flags = VII_F_READ_ONLY,
        .kind = VII_T_S32,
        .is_public = 1,
        .value_count = 6,
        .values = vi_system_node_class_t_values,
    },
    {
        .id = 2,
        .flags = VII_F_READ_ONLY | VII_F_SUBSCRIBABLE,
        .kind = VII_T_VERSION,
        .is_public = 1,
    },
    {
        .id = 3,
        .flags = VII_F_READ_ONLY | VII_F_SUBSCRIBABLE,
        .kind = VII_T_STRUCT,
        .is_public = 1,
        .struct_index = 0,
        .size = sizeof(vi_system_state_t),
    },
    {
        .id = 4,
        .flags = VII_F_READ_ONLY,
        .kind = VII_T_TEXT,
        .is_public = 1,
    },
};

static const vuint8_t system_attr_by_id[] = {0, 1, 2, 3, 4};

const struct vii_class vii_classes[V_CLASS_ID_MAX + 1] = {
    [V_CLASS_CONTRACT] =
        {
            .attr_by_id = contract_attr_by_id,
            .attrs = contract_attrs,
            .attr_id_max = 4,
            .external_verbs = (1u << V_VERB_GET) | (1u << V_VERB_SUBSCRIBE) | (1u << V_VERB_UNSUBSCRIBE),
            .body =
                {
                    [V_VERB_CREATE] = vlib_contract_create,
                    [V_VERB_GET] = vlib_contract_get,
                    [V_VERB_SET] = vlib_contract_set,
                    [V_VERB_DELETE] = vlib_contract_delete,
                    [V_VERB_SUBSCRIBE] = vlib_contract_subscribe,
                    [V_VERB_UNSUBSCRIBE] = vlib_contract_unsubscribe,
                    [V_VERB_NOTIFY] = vlib_contract_notify,
                },
            .attr_count = 2,
            .singleton = 1,
        },
    [V_CLASS_IDENTITY] =
        {
            .attr_by_id = identity_attr_by_id,
            .attrs = identity_attrs,
            .attr_id_max = 2,
            .external_verbs = (1u << V_VERB_GET) | (1u << V_VERB_SUBSCRIBE) | (1u << V_VERB_UNSUBSCRIBE),
            .body =
                {
                    [V_VERB_CREATE] = vlib_identity_create,
                    [V_VERB_GET] = vlib_identity_get,
                    [V_VERB_SET] = vlib_identity_set,
                    [V_VERB_DELETE] = vlib_identity_delete,
                    [V_VERB_SUBSCRIBE] = vlib_identity_subscribe,
                    [V_VERB_UNSUBSCRIBE] = vlib_identity_unsubscribe,
                    [V_VERB_NOTIFY] = vlib_identity_notify,
                },
            .attr_count = 2,
            .singleton = 0,
        },
    [V_CLASS_PLUGIN] =
        {
            .attr_by_id = plugin_attr_by_id,
            .attrs = plugin_attrs,
            .attr_id_max = 1,
            .external_verbs = (1u << V_VERB_GET) | (1u << V_VERB_SUBSCRIBE) | (1u << V_VERB_UNSUBSCRIBE),
            .body =
                {
                    [V_VERB_CREATE] = vlib_plugin_create,
                    [V_VERB_GET] = vlib_plugin_get,
                    [V_VERB_SET] = vlib_plugin_set,
                    [V_VERB_DELETE] = vlib_plugin_delete,
                    [V_VERB_SUBSCRIBE] = vlib_plugin_subscribe,
                    [V_VERB_UNSUBSCRIBE] = vlib_plugin_unsubscribe,
                    [V_VERB_NOTIFY] = vlib_plugin_notify,
                },
            .attr_count = 1,
            .singleton = 1,
        },
    [V_CLASS_SECURITY] =
        {
            .attr_by_id = security_attr_by_id,
            .attrs = security_attrs,
            .attr_id_max = 2,
            .external_verbs = (1u << V_VERB_GET) | (1u << V_VERB_SUBSCRIBE) | (1u << V_VERB_UNSUBSCRIBE),
            .body =
                {
                    [V_VERB_CREATE] = vlib_security_create,
                    [V_VERB_GET] = vlib_security_get,
                    [V_VERB_SET] = vlib_security_set,
                    [V_VERB_DELETE] = vlib_security_delete,
                    [V_VERB_SUBSCRIBE] = vlib_security_subscribe,
                    [V_VERB_UNSUBSCRIBE] = vlib_security_unsubscribe,
                    [V_VERB_NOTIFY] = vlib_security_notify,
                },
            .attr_count = 2,
            .singleton = 1,
        },
    [V_CLASS_STORAGE] =
        {
            .attr_by_id = storage_attr_by_id,
            .attrs = storage_attrs,
            .attr_id_max = 2,
            .external_verbs = (1u << V_VERB_GET) | (1u << V_VERB_SUBSCRIBE) | (1u << V_VERB_UNSUBSCRIBE),
            .body =
                {
                    [V_VERB_CREATE] = vlib_storage_create,
                    [V_VERB_GET] = vlib_storage_get,
                    [V_VERB_SET] = vlib_storage_set,
                    [V_VERB_DELETE] = vlib_storage_delete,
                    [V_VERB_SUBSCRIBE] = vlib_storage_subscribe,
                    [V_VERB_UNSUBSCRIBE] = vlib_storage_unsubscribe,
                    [V_VERB_NOTIFY] = vlib_storage_notify,
                },
            .attr_count = 2,
            .singleton = 0,
        },
    [V_CLASS_SYSTEM] =
        {
            .attr_by_id = system_attr_by_id,
            .attrs = system_attrs,
            .attr_id_max = 4,
            .external_verbs = (1u << V_VERB_GET) | (1u << V_VERB_SUBSCRIBE) | (1u << V_VERB_UNSUBSCRIBE),
            .body =
                {
                    [V_VERB_CREATE] = vlib_system_create,
                    [V_VERB_GET] = vlib_system_get,
                    [V_VERB_SET] = vlib_system_set,
                    [V_VERB_DELETE] = vlib_system_delete,
                    [V_VERB_SUBSCRIBE] = vlib_system_subscribe,
                    [V_VERB_UNSUBSCRIBE] = vlib_system_unsubscribe,
                    [V_VERB_NOTIFY] = vlib_system_notify,
                },
            .attr_count = 4,
            .singleton = 1,
        },
};

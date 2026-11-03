/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#include "vti/dispatch.h"

#include "core/core.h"
#include "lock/store.h"
#include "platform/memory.h"
#include "vii/class/identity.h"
#include "vii/class/security.h"
#include "vii/class/storage.h"
#include "vii/class/system.h"
#include "vii/class/wasm.h"

enum { VTA_VERB_SLOTS = 7 };

/**
 * @brief Arguments copied into the lock-map body for one vii call.
 */
struct dispatch_args {
    vta_msg_t *call;
};

/**
 * @brief Look up the verb function for the class and call it.
 *
 * @param[in] user Pointer to struct dispatch_args
 * @return The vii status, or V_STATUS_NOT_IMPLEMENTED when unknown.
 */
static v_status_t dispatch_body(void *user)
{
    struct dispatch_args *args = (struct dispatch_args *)user;
    vta_msg_t *call = args->call;
    vedam_t *node = vedam_node();
    vta_verb_fn fn;

    if (node == NULL || node->verbs == NULL) {
        return V_STATUS_UNINITIALIZED;
    }
    if ((unsigned)call->class_id > (unsigned)V_CLASS_ID_MAX) {
        return V_STATUS_NOT_IMPLEMENTED;
    }
    if ((unsigned)call->verb >= (unsigned)VTA_VERB_SLOTS) {
        return V_STATUS_NOT_IMPLEMENTED;
    }

    fn = node->verbs[call->class_id].fn[call->verb];
    if (fn == NULL) {
        return V_STATUS_NOT_IMPLEMENTED;
    }
    return fn(call->origin, call->major, call->minor, call->msg);
}

/**
 * @brief Fill the per-class verb table on @p node.
 *
 * @param[in,out] node Process node that owns the table
 * @return V_STATUS_SUCCESS, or V_STATUS_NO_MEMORY when the table cannot be allocated.
 */
v_status_t vta_dispatch_table_init(vedam_t *node)
{
    struct vta_class_verbs *table;

    if (node == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }

    table = vplatform_calloc((size_t)V_CLASS_ID_MAX + 1u, sizeof(*table));
    if (table == NULL) {
        return V_STATUS_NO_MEMORY;
    }

    table[V_CLASS_IDENTITY].fn[V_VERB_CREATE] = vii_identity_create;
    table[V_CLASS_IDENTITY].fn[V_VERB_GET] = vii_identity_get;
    table[V_CLASS_IDENTITY].fn[V_VERB_SET] = vii_identity_set;
    table[V_CLASS_IDENTITY].fn[V_VERB_DELETE] = vii_identity_delete;
    table[V_CLASS_IDENTITY].fn[V_VERB_SUBSCRIBE] = vii_identity_subscribe;
    table[V_CLASS_IDENTITY].fn[V_VERB_UNSUBSCRIBE] = vii_identity_unsubscribe;
    table[V_CLASS_IDENTITY].fn[V_VERB_NOTIFY] = vii_identity_notify;
    table[V_CLASS_SECURITY].fn[V_VERB_CREATE] = vii_security_create;
    table[V_CLASS_SECURITY].fn[V_VERB_GET] = vii_security_get;
    table[V_CLASS_SECURITY].fn[V_VERB_SET] = vii_security_set;
    table[V_CLASS_SECURITY].fn[V_VERB_DELETE] = vii_security_delete;
    table[V_CLASS_SECURITY].fn[V_VERB_SUBSCRIBE] = vii_security_subscribe;
    table[V_CLASS_SECURITY].fn[V_VERB_UNSUBSCRIBE] = vii_security_unsubscribe;
    table[V_CLASS_SECURITY].fn[V_VERB_NOTIFY] = vii_security_notify;
    table[V_CLASS_STORAGE].fn[V_VERB_CREATE] = vii_storage_create;
    table[V_CLASS_STORAGE].fn[V_VERB_GET] = vii_storage_get;
    table[V_CLASS_STORAGE].fn[V_VERB_SET] = vii_storage_set;
    table[V_CLASS_STORAGE].fn[V_VERB_DELETE] = vii_storage_delete;
    table[V_CLASS_STORAGE].fn[V_VERB_SUBSCRIBE] = vii_storage_subscribe;
    table[V_CLASS_STORAGE].fn[V_VERB_UNSUBSCRIBE] = vii_storage_unsubscribe;
    table[V_CLASS_STORAGE].fn[V_VERB_NOTIFY] = vii_storage_notify;
    table[V_CLASS_SYSTEM].fn[V_VERB_CREATE] = vii_system_create;
    table[V_CLASS_SYSTEM].fn[V_VERB_GET] = vii_system_get;
    table[V_CLASS_SYSTEM].fn[V_VERB_SET] = vii_system_set;
    table[V_CLASS_SYSTEM].fn[V_VERB_DELETE] = vii_system_delete;
    table[V_CLASS_SYSTEM].fn[V_VERB_SUBSCRIBE] = vii_system_subscribe;
    table[V_CLASS_SYSTEM].fn[V_VERB_UNSUBSCRIBE] = vii_system_unsubscribe;
    table[V_CLASS_SYSTEM].fn[V_VERB_NOTIFY] = vii_system_notify;
    table[V_CLASS_WASM].fn[V_VERB_CREATE] = vii_wasm_create;
    table[V_CLASS_WASM].fn[V_VERB_GET] = vii_wasm_get;
    table[V_CLASS_WASM].fn[V_VERB_SET] = vii_wasm_set;
    table[V_CLASS_WASM].fn[V_VERB_DELETE] = vii_wasm_delete;
    table[V_CLASS_WASM].fn[V_VERB_SUBSCRIBE] = vii_wasm_subscribe;
    table[V_CLASS_WASM].fn[V_VERB_UNSUBSCRIBE] = vii_wasm_unsubscribe;
    table[V_CLASS_WASM].fn[V_VERB_NOTIFY] = vii_wasm_notify;

    node->verbs = table;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Free the verb table on @p node.
 *
 * @param[in,out] node Process node that owns the table
 */
void vta_dispatch_table_fini(vedam_t *node)
{
    if (node == NULL || node->verbs == NULL) {
        return;
    }
    vplatform_free(node->verbs);
    node->verbs = NULL;
}

/**
 * @brief Enter the node for @p call under the Wait-Die lock map.
 *
 * Local vi_*, init, and a transport receive that already holds a vi_msg_t
 * call this. vedam_lock_invoke takes the locks, then the class verb runs.
 * A library that needs another vertical calls this again with internal
 * origin. That call joins the attempt already running on the thread. A
 * Wait-Die die returns VEDAM_LOCK_RESTART and the outermost attempt starts
 * again. A send uses vta_call after that attempt releases its locks.
 *
 * @param[in,out] call Transport message (class, verb, origin, version, and vi_msg_t)
 * @return The vii status, or V_STATUS_NOT_IMPLEMENTED when the class or verb is unknown.
 */
v_status_t vta_dispatch(vta_msg_t *call)
{
    struct dispatch_args args;
    vi_msg_t *msg;

    if (vedam_node() == NULL) {
        return V_STATUS_UNINITIALIZED;
    }
    if (call == NULL || call->msg == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }

    msg = call->msg;
    args.call = call;
    return vedam_lock_invoke(call->class_id, msg->key, msg->key_len, call->verb, call->origin,
                             msg->attr_count, msg->attr_list, msg->status_list, dispatch_body,
                             &args);
}

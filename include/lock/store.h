/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VEDAM_LOCK_STORE_H
#define VEDAM_LOCK_STORE_H

#include "vii/types.h"

struct vedam_node;

/**
 * @brief Nested local call should be retried by the outermost vedam_lock_invoke.
 *
 * vedam_lock_invoke does not return this value to the outermost caller.
 */
enum { VEDAM_LOCK_RESTART = 1 };

/**
 * @brief Body invoked after the attribute locks for this call are held.
 */
typedef v_status_t (*vedam_lock_body_fn)(void *user);

/**
 * @brief Create the process lock map on @p node.
 *
 * @param[in,out] node Node allocated by vedam_init
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vedam_lockdb_open(struct vedam_node *node);

/**
 * @brief Free every object lock record on @p node.
 *
 * @param[in,out] node Node whose lock map is open
 */
void vedam_lockdb_close(struct vedam_node *node);

/**
 * @brief Hold the locks for one local call, run @p body, and release them at depth zero.
 *
 * Create locks the object's existence. Get takes a shared hold on each attribute.
 * Set takes an exclusive hold on each attribute. Delete locks existence and every
 * attribute lock that already exists. Subscribe, unsubscribe, and notify take
 * the same shared attribute locks as get. A null key is the singleton key.
 *
 * @param[in]     class_id   Class identifier
 * @param[in]     key        Object key bytes, or NULL for the singleton
 * @param[in]     key_len    Length of @p key
 * @param[in]     verb       Verb identifier
 * @param[in]     origin     External or internal
 * @param[in]     attr_count   Number of attributes
 * @param[in,out] attr_list    Attribute array
 * @param[out]    status_list  Per-attribute status codes, or NULL when unused
 * @param[in]     body         Class body
 * @param[in]     user         Pointer passed to @p body
 * @return The body status. VEDAM_LOCK_RESTART is consumed by the outermost call.
 */
/**
 * @brief Non-zero while this thread is inside vedam_lock_invoke.
 */
int vedam_lock_in_attempt(void);

/**
 * @brief Number of lock objects allocated on a miss since the lock map opened.
 *
 * Hits do not increment this counter. Used by tests to assert the hit path.
 *
 * @return The miss count, or 0 when the node is not open.
 */
unsigned long vedam_lock_object_misses(void);

v_status_t vedam_lock_invoke(v_class_id_t class_id, const void *key, vuint32_t key_len,
                             v_verb_t verb, v_origin_t origin, vuint32_t attr_count,
                             v_attr_t *attr_list, v_status_t *status_list, vedam_lock_body_fn body,
                             void *user);

#endif

/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "lock/store.h"

#include "core/node.h"

/**
 * @brief Leave the lock map empty.
 *
 * @param[in,out] node Node allocated by vedam_init
 * @return V_STATUS_SUCCESS, or V_STATUS_INVALID_PARAMETER when @p node has no class table.
 */
v_status_t vedam_lockdb_open(struct vedam_node *node)
{
    if (node == NULL || node->classes == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    return V_STATUS_SUCCESS;
}

/**
 * @brief Release nothing. This build does not allocate a lock map.
 *
 * @param[in,out] node Node whose lock map would be open
 */
void vedam_lockdb_close(struct vedam_node *node)
{
    (void)node;
}

/**
 * @brief Report that this thread holds no Wait-Die attempt.
 *
 * @return 0.
 */
int vedam_lock_in_attempt(void)
{
    return 0;
}

/**
 * @brief Report that no lock object was allocated.
 *
 * @return 0.
 */
unsigned long vedam_lock_object_misses(void)
{
    return 0;
}

/**
 * @brief Run @p body directly.
 *
 * @param[in]     class_id    Unused
 * @param[in]     key         Unused
 * @param[in]     key_len     Unused
 * @param[in]     verb        Unused
 * @param[in]     origin      Unused
 * @param[in]     attr_count  Unused
 * @param[in,out] attr_list   Unused
 * @param[out]    status_list Unused
 * @param[in]     body        Class body
 * @param[in]     user        Pointer passed to @p body
 * @return The body status, or V_STATUS_INVALID_PARAMETER when @p body is NULL.
 */
v_status_t vedam_lock_invoke(v_class_id_t class_id, const void *key, vuint32_t key_len,
                             v_verb_t verb, v_origin_t origin, vuint32_t attr_count,
                             v_attr_t *attr_list, v_status_t *status_list, vedam_lock_body_fn body,
                             void *user)
{
    (void)class_id;
    (void)key;
    (void)key_len;
    (void)verb;
    (void)origin;
    (void)attr_count;
    (void)attr_list;
    (void)status_list;
    if (body == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    return body(user);
}

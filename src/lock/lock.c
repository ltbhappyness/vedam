/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "lock/lock.h"

#include "platform/platform.h"
#include "util/log.h"
#include "wwlock.h"
#include "wwlock_futex.h"

struct vedam_lock_group {
    wwlock_class_t *klass;
};

struct vedam_lock {
    wwlock_t *inner;
};

struct hold {
    struct vedam_lock *lock;
    struct hold *next;
};

static _Thread_local struct hold *tls_holds;

/**
 * @brief Clear every hold record on the current attempt.
 */
static void holds_clear(void)
{
    while (tls_holds != NULL) {
        struct hold *hold = tls_holds;

        tls_holds = hold->next;
        vplatform_free(hold);
    }
}

/**
 * @brief Record one lock hold on the current attempt.
 * @param[in] lock Argument lock
 * @return Result of holds_add.
 */
static int holds_add(struct vedam_lock *lock)
{
    struct hold *hold;

    hold = vplatform_calloc(1, sizeof(*hold));
    if (hold == NULL) {
        return -1;
    }
    hold->lock = lock;
    hold->next = tls_holds;
    tls_holds = hold;
    return 0;
}

/**
 * @brief Drop one hold record that matches @p lock.
 * @param[in] lock Argument lock
 */
static void holds_drop_one(struct vedam_lock *lock)
{
    struct hold **link = &tls_holds;

    while (*link != NULL) {
        if ((*link)->lock == lock) {
            struct hold *hold = *link;

            *link = hold->next;
            vplatform_free(hold);
            return;
        }
        link = &(*link)->next;
    }
}

/**
 * @brief Map a Wait-Die result to a Vedam status code.
 * @param[in] rc Argument rc
 * @return Result of map_fail.
 */
static v_status_t map_fail(int rc)
{
    if (rc == WWLOCK_NOMEM) {
        return V_STATUS_NO_MEMORY;
    }
    return V_STATUS_INVALID_PARAMETER;
}

/**
 * @brief Open the process-wide Wait-Die lock group.
 * @param[in] group Argument group
 * @return Result of vedam_lock_group_open.
 */
v_status_t vedam_lock_group_open(vedam_lock_group_t **group)
{
    vedam_lock_group_t *created;
    wwlock_park_t park;
    wwlock_class_t *klass;
    int rc;

    if (group == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    rc = wwlock_futex_fill(&park);
    if (rc != WWLOCK_OK) {
        return V_STATUS_INVALID_PARAMETER;
    }
    rc = wwlock_class_init(&klass, &park);
    if (rc != WWLOCK_OK) {
        return map_fail(rc);
    }
    created = vplatform_calloc(1, sizeof(*created));
    if (created == NULL) {
        wwlock_class_deinit(klass);
        return V_STATUS_NO_MEMORY;
    }
    created->klass = klass;
    *group = created;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Close the process-wide Wait-Die lock group.
 * @param[in] group Argument group
 */
void vedam_lock_group_close(vedam_lock_group_t *group)
{
    int rc;

    if (group == NULL) {
        return;
    }
    rc = wwlock_class_deinit(group->klass);
    if (rc != WWLOCK_OK) {
        vlog_error("lock group still in use");
        return;
    }
    vplatform_free(group);
}

/**
 * @brief Create one Wait-Die lock object.
 * @param[in] lock Argument lock
 * @param[in] group Argument group
 * @return Result of vedam_lock_create.
 */
v_status_t vedam_lock_create(vedam_lock_t **lock, vedam_lock_group_t *group)
{
    vedam_lock_t *created;
    wwlock_t *inner;
    int rc;

    if (lock == NULL || group == NULL || group->klass == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    rc = wwlock_init(&inner, group->klass);
    if (rc != WWLOCK_OK) {
        return map_fail(rc);
    }
    created = vplatform_calloc(1, sizeof(*created));
    if (created == NULL) {
        wwlock_deinit(inner);
        return V_STATUS_NO_MEMORY;
    }
    created->inner = inner;
    *lock = created;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Destroy one Wait-Die lock object.
 * @param[in] lock Argument lock
 * @return Result of vedam_lock_destroy.
 */
v_status_t vedam_lock_destroy(vedam_lock_t *lock)
{
    int rc;

    if (lock == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    rc = wwlock_deinit(lock->inner);
    if (rc == WWLOCK_BUSY) {
        return V_STATUS_OBJECT_IN_USE;
    }
    if (rc != WWLOCK_OK) {
        return V_STATUS_INVALID_PARAMETER;
    }
    vplatform_free(lock);
    return V_STATUS_SUCCESS;
}

/**
 * @brief Acquire @p lock for the current attempt.
 * @param[in] lock Argument lock
 * @param[in] exclusive Argument exclusive
 * @return Result of vedam_lock_acquire.
 */
int vedam_lock_acquire(vedam_lock_t *lock, int exclusive)
{
    int added;
    int rc;

    if (lock == NULL || lock->inner == NULL) {
        return (int)V_STATUS_INVALID_PARAMETER;
    }

    rc = exclusive ? wwlock_wrlock(lock->inner) : wwlock_rdlock(lock->inner);
    if (rc == WWLOCK_OK) {
        added = holds_add(lock);
        if (added != 0) {
            wwlock_unlock(lock->inner);
            return (int)V_STATUS_NO_MEMORY;
        }
        return VEDAM_LOCK_HELD;
    }
    if (rc == WWLOCK_ALREADY) {
        return VEDAM_LOCK_JOINED;
    }
    if (rc == WWLOCK_DIE) {
        holds_clear();
        added = holds_add(lock);
        if (added != 0) {
            wwlock_unlock(lock->inner);
            return (int)V_STATUS_NO_MEMORY;
        }
        return VEDAM_LOCK_RETRY;
    }
    return (int)map_fail(rc);
}

/**
 * @brief Release @p lock for the current attempt.
 * @param[in] lock Argument lock
 * @return Result of vedam_lock_release.
 */
v_status_t vedam_lock_release(vedam_lock_t *lock)
{
    int rc;

    if (lock == NULL || lock->inner == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    rc = wwlock_unlock(lock->inner);
    if (rc != WWLOCK_OK) {
        return V_STATUS_INVALID_PARAMETER;
    }
    holds_drop_one(lock);
    return V_STATUS_SUCCESS;
}

/**
 * @brief Release every lock held by the current attempt.
 */
void vedam_lock_release_attempt(void)
{
    while (tls_holds != NULL) {
        struct vedam_lock *lock = tls_holds->lock;

        holds_drop_one(lock);
        if (lock != NULL && lock->inner != NULL) {
            wwlock_unlock(lock->inner);
        }
    }
}

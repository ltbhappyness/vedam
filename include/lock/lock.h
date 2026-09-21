/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VEDAM_LOCK_H
#define VEDAM_LOCK_H

#include "vii/types.h"

/**
 * @brief One Wait-Die group.
 *
 * vedam_lock_group_open creates the object. Callers see only the pointer.
 * The library keeps one counter inside it. The first acquire of an attempt
 * adds one to that counter and keeps the value until the last release.
 * That value is the ticket. A smaller ticket is older. Every lock created
 * from this group uses that one counter, so threads that start attempts at
 * the same time queue on it even when they lock different objects.
 */
typedef struct vedam_lock_group vedam_lock_group_t;

/** @brief One lock created from a vedam_lock_group_t. */
typedef struct vedam_lock vedam_lock_t;

/**
 * @brief Results from vedam_lock_acquire that still mean the lock is held.
 *
 * VEDAM_LOCK_HELD means this call added one hold. Release that hold once.
 * VEDAM_LOCK_JOINED means the thread already held the lock, and this call
 * added no hold. VEDAM_LOCK_RETRY means this lock is held, earlier holds of
 * this attempt were released, and the outermost attempt must take its locks
 * again from the first one. Do not release the retried lock before that.
 * Any other return is a v_status_t and the lock was not taken.
 */
enum {
    VEDAM_LOCK_HELD = 0,
    VEDAM_LOCK_JOINED = 2,
    VEDAM_LOCK_RETRY = 3,
};

/**
 * @brief Create the process lock group.
 *
 * @param[out] group Receives the group
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vedam_lock_group_open(vedam_lock_group_t **group);

/**
 * @brief Free a group after every lock created from it has been destroyed.
 *
 * @param[in] group Group object
 */
void vedam_lock_group_close(vedam_lock_group_t *group);

/**
 * @brief Create an unlocked lock in @p group.
 *
 * @param[out] lock  Receives the lock
 * @param[in]  group Group from vedam_lock_group_open
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vedam_lock_create(vedam_lock_t **lock, vedam_lock_group_t *group);

/**
 * @brief Free a lock that is not held and has no waiter.
 *
 * @param[in] lock Lock object
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_OBJECT_IN_USE.
 */
v_status_t vedam_lock_destroy(vedam_lock_t *lock);

/**
 * @brief Hold @p lock on the calling thread.
 *
 * @param[in,out] lock      Lock object
 * @param[in]     exclusive Non-zero for a writer. Zero for a reader.
 * @return VEDAM_LOCK_HELD, VEDAM_LOCK_JOINED, VEDAM_LOCK_RETRY,
 *         V_STATUS_NO_MEMORY, or V_STATUS_INVALID_PARAMETER.
 */
int vedam_lock_acquire(vedam_lock_t *lock, int exclusive);

/**
 * @brief Drop one hold added by vedam_lock_acquire.
 *
 * @param[in,out] lock Lock object
 * @return V_STATUS_SUCCESS, or V_STATUS_INVALID_PARAMETER when this thread does not hold it.
 */
v_status_t vedam_lock_release(vedam_lock_t *lock);

/**
 * @brief Drop every hold this thread recorded in the current attempt.
 */
void vedam_lock_release_attempt(void);

#endif

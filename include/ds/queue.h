/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VDS_QUEUE_H
#define VDS_QUEUE_H

#include "ds/list.h"

/**
 * @brief First-in first-out queue of pointers. Nodes are allocated on enqueue.
 */
typedef struct vds_queue {
    struct vds_list head;
    vuint32_t count;
} vds_queue_t;

/**
 * @brief Prepare an empty queue.
 *
 * @param[out] queue Queue object
 */
void vds_queue_init(vds_queue_t *queue);

/**
 * @brief Enqueue @p item at the tail.
 *
 * @param[in,out] queue Queue object
 * @param[in]     item  Caller-owned pointer
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_queue_enqueue(vds_queue_t *queue, void *item);

/**
 * @brief Dequeue the oldest item.
 *
 * @param[in,out] queue Queue object
 * @param[out]    item  Receives the pointer
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_queue_dequeue(vds_queue_t *queue, void **item);

/**
 * @brief Free remaining nodes. Pointed-to objects are left to the caller.
 *
 * @param[in,out] queue Queue object
 */
void vds_queue_clear(vds_queue_t *queue);

#endif

/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "ds/queue.h"

#include "platform/memory.h"

struct vds_queue_node {
    struct vds_list link;
    void *item;
};

/**
 * @brief Prepare an empty queue.
 *
 * @param[out] queue Queue object
 */
void vds_queue_init(vds_queue_t *queue)
{
    if (queue == NULL) {
        return;
    }
    vds_list_init(&queue->head);
    queue->count = 0;
}

/**
 * @brief Enqueue @p item at the tail.
 *
 * @param[in,out] queue Queue object
 * @param[in]     item  Caller-owned pointer
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_queue_enqueue(vds_queue_t *queue, void *item)
{
    struct vds_queue_node *node;

    if (queue == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    node = vplatform_calloc(1, sizeof(*node));
    if (node == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    node->item = item;
    vds_list_add_tail(&node->link, &queue->head);
    queue->count++;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Dequeue the oldest item.
 *
 * @param[in,out] queue Queue object
 * @param[out]    item  Receives the pointer
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_queue_dequeue(vds_queue_t *queue, void **item)
{
    struct vds_queue_node *node;

    if (queue == NULL || item == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (vds_list_empty(&queue->head)) {
        return V_STATUS_ITEM_NOT_FOUND;
    }
    node = vds_container_of(queue->head.next, struct vds_queue_node, link);
    vds_list_del(&node->link);
    *item = node->item;
    vplatform_free(node);
    queue->count--;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Free remaining nodes. Pointed-to objects are left to the caller.
 *
 * @param[in,out] queue Queue object
 */
void vds_queue_clear(vds_queue_t *queue)
{
    struct vds_list *pos;
    struct vds_list *tmp;

    if (queue == NULL) {
        return;
    }
    vds_list_for_each_safe (pos, tmp, &queue->head) {
        struct vds_queue_node *node = vds_container_of(pos, struct vds_queue_node, link);

        vds_list_del(pos);
        vplatform_free(node);
    }
    queue->count = 0;
}

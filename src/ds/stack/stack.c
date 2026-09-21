/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "ds/stack.h"

#include "platform/memory.h"

struct vds_stack_node {
    struct vds_list link;
    void *item;
};

/**
 * @brief Prepare an empty stack.
 *
 * @param[out] stack Stack object
 */
void vds_stack_init(vds_stack_t *stack)
{
    if (stack == NULL) {
        return;
    }
    vds_list_init(&stack->head);
    stack->count = 0;
}

/**
 * @brief Push @p item. The pointer is stored, not copied.
 *
 * @param[in,out] stack Stack object
 * @param[in]     item  Caller-owned pointer
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_stack_push(vds_stack_t *stack, void *item)
{
    struct vds_stack_node *node;

    if (stack == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    node = (struct vds_stack_node *)vplatform_calloc(1, sizeof(*node));
    if (node == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    node->item = item;
    vds_list_add(&node->link, &stack->head);
    stack->count++;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Pop the most recently pushed item.
 *
 * @param[in,out] stack Stack object
 * @param[out]    item  Receives the pointer
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_stack_pop(vds_stack_t *stack, void **item)
{
    struct vds_stack_node *node;

    if (stack == NULL || item == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (vds_list_empty(&stack->head)) {
        return V_STATUS_ITEM_NOT_FOUND;
    }
    node = vds_container_of(stack->head.next, struct vds_stack_node, link);
    vds_list_del(&node->link);
    *item = node->item;
    vplatform_free(node);
    stack->count--;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Free remaining nodes. Pointed-to objects are left to the caller.
 *
 * @param[in,out] stack Stack object
 */
void vds_stack_clear(vds_stack_t *stack)
{
    struct vds_list *pos;
    struct vds_list *tmp;

    if (stack == NULL) {
        return;
    }
/**
 * @brief Walk each element in a circular list while allowing removal.
 * @param[in] pos Argument pos
 * @param[in] tmp Argument tmp
 * @param[in] stack->head Argument stack->head
 * @return Result of vds_list_for_each_safe.
 */
    vds_list_for_each_safe(pos, tmp, &stack->head) {
        struct vds_stack_node *node = vds_container_of(pos, struct vds_stack_node, link);

        vds_list_del(pos);
        vplatform_free(node);
    }
    stack->count = 0;
}

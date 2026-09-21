/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VDS_STACK_H
#define VDS_STACK_H

#include "ds/list.h"

/**
 * @brief Last-in first-out stack of pointers. Nodes are allocated on push.
 */
typedef struct vds_stack {
    struct vds_list head;
    vuint32_t count;
} vds_stack_t;

/**
 * @brief Prepare an empty stack.
 *
 * @param[out] stack Stack object
 */
void vds_stack_init(vds_stack_t *stack);

/**
 * @brief Push @p item. The pointer is stored, not copied.
 *
 * @param[in,out] stack Stack object
 * @param[in]     item  Caller-owned pointer
 * @return V_STATUS_SUCCESS, V_STATUS_INVALID_PARAMETER, or V_STATUS_NO_MEMORY.
 */
v_status_t vds_stack_push(vds_stack_t *stack, void *item);

/**
 * @brief Pop the most recently pushed item.
 *
 * @param[in,out] stack Stack object
 * @param[out]    item  Receives the pointer
 * @return V_STATUS_SUCCESS or V_STATUS_ITEM_NOT_FOUND.
 */
v_status_t vds_stack_pop(vds_stack_t *stack, void **item);

/**
 * @brief Free remaining nodes. Pointed-to objects are left to the caller.
 *
 * @param[in,out] stack Stack object
 */
void vds_stack_clear(vds_stack_t *stack);

#endif

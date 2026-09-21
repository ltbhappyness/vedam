/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VDS_LIST_H
#define VDS_LIST_H

#include "vi/types.h"

#include <stddef.h>

/**
 * @brief Circular doubly linked list node. Embed this in the object that is linked.
 */
struct vds_list {
    struct vds_list *next;
    struct vds_list *prev;
};

/**
 * @brief Point @p head at itself so the list is empty.
 *
 * @param[out] head List head
 */
static inline void vds_list_init(struct vds_list *head)
{
    head->next = head;
    head->prev = head;
}

/**
 * @brief Insert @p node between @p prev and @p next.
 *
 * @param[in,out] node Node to insert
 * @param[in,out] prev Predecessor
 * @param[in,out] next Successor
 */
static inline void vds_list_insert(struct vds_list *node, struct vds_list *prev,
                                   struct vds_list *next)
{
    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next = node;
}

/**
 * @brief Insert @p node just after @p head.
 *
 * @param[in,out] node Node to insert
 * @param[in,out] head List head
 */
static inline void vds_list_add(struct vds_list *node, struct vds_list *head)
{
    vds_list_insert(node, head, head->next);
}

/**
 * @brief Insert @p node just before @p head.
 *
 * @param[in,out] node Node to insert
 * @param[in,out] head List head
 */
static inline void vds_list_add_tail(struct vds_list *node, struct vds_list *head)
{
    vds_list_insert(node, head->prev, head);
}

/**
 * @brief Unlink @p node. The node pointers are cleared.
 *
 * @param[in,out] node Node to remove
 */
static inline void vds_list_del(struct vds_list *node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = NULL;
    node->prev = NULL;
}

/**
 * @brief Report whether @p head has no nodes.
 *
 * @param[in] head List head
 * @return 1 when empty, 0 otherwise.
 */
static inline int vds_list_empty(const struct vds_list *head)
{
    return head->next == head;
}

/**
 * @brief Recover the object that embeds @p ptr.
 */
#define vds_container_of(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))

/**
 * @brief Walk @p head. Do not unlink the current node.
 */
#define vds_list_for_each(pos, head) for ((pos) = (head)->next; (pos) != (head); (pos) = (pos)->next)

/**
 * @brief Walk @p head. @p tmp keeps the walk valid when the current node is unlinked.
 */
#define vds_list_for_each_safe(pos, tmp, head)                                                     \
    for ((pos) = (head)->next, (tmp) = (pos)->next; (pos) != (head);                               \
         (pos) = (tmp), (tmp) = (pos)->next)

/**
 * @brief Count nodes on @p head. The head itself is not counted.
 *
 * @param[in] head List head
 * @return Number of nodes.
 */
vuint32_t vds_list_count(const struct vds_list *head);

#endif

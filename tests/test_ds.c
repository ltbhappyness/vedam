/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "ds/hashkv.h"
#include "ds/index.h"
#include "ds/list.h"
#include "ds/queue.h"
#include "ds/singleton.h"
#include "ds/stack.h"

#include <stdio.h>

static int fail(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    return 1;
}

static int test_list(void)
{
    struct vds_list head;
    struct vds_list a;
    struct vds_list b;

    vds_list_init(&head);
    vds_list_init(&a);
    vds_list_init(&b);
    vds_list_add(&a, &head);
    vds_list_add_tail(&b, &head);
    if (vds_list_count(&head) != 2 || head.next != &a || head.prev != &b) {
        return fail("list order");
    }
    vds_list_del(&a);
    if (vds_list_count(&head) != 1 || a.next != NULL) {
        return fail("list del");
    }
    return 0;
}

static int test_stack(void)
{
    vds_stack_t stack;
    void *item = NULL;
    int first = 1;
    int second = 2;

    vds_stack_init(&stack);
    if (vds_stack_push(&stack, &first) != V_STATUS_SUCCESS ||
        vds_stack_push(&stack, &second) != V_STATUS_SUCCESS) {
        return fail("stack push");
    }
    if (vds_stack_pop(&stack, &item) != V_STATUS_SUCCESS || item != &second) {
        return fail("stack pop");
    }
    vds_stack_clear(&stack);
    if (vds_stack_pop(&stack, &item) != V_STATUS_ITEM_NOT_FOUND) {
        return fail("stack empty");
    }
    return 0;
}

static int test_queue(void)
{
    vds_queue_t queue;
    void *item = NULL;
    int first = 1;
    int second = 2;

    vds_queue_init(&queue);
    if (vds_queue_enqueue(&queue, &first) != V_STATUS_SUCCESS ||
        vds_queue_enqueue(&queue, &second) != V_STATUS_SUCCESS) {
        return fail("queue enqueue");
    }
    if (vds_queue_dequeue(&queue, &item) != V_STATUS_SUCCESS || item != &first) {
        return fail("queue dequeue");
    }
    vds_queue_clear(&queue);
    return 0;
}

static int test_stores(void)
{
    vds_singleton_t *single = NULL;
    vds_index_t *index = NULL;
    vds_hashkv_t *kv = NULL;
    void *got = NULL;
    int value = 7;
    int other = 9;

    if (vds_singleton_create(&single) != V_STATUS_SUCCESS ||
        vds_singleton_set(single, &value) != V_STATUS_SUCCESS ||
        vds_singleton_get(single, &got) != V_STATUS_SUCCESS || got != &value) {
        return fail("singleton");
    }
    vds_singleton_destroy(single);

    if (vds_index_create(&index, 2) != V_STATUS_SUCCESS ||
        vds_index_set(index, 5, &other) != V_STATUS_SUCCESS ||
        vds_index_get(index, 5, &got) != V_STATUS_SUCCESS || got != &other) {
        return fail("index");
    }
    if (vds_index_del(index, 5) != V_STATUS_SUCCESS ||
        vds_index_get(index, 5, &got) != V_STATUS_ITEM_NOT_FOUND) {
        return fail("index del");
    }
    vds_index_destroy(index);

    if (vds_hashkv_create(&kv) != V_STATUS_SUCCESS ||
        vds_hashkv_set(kv, "node", 4, &value) != V_STATUS_SUCCESS ||
        vds_hashkv_get(kv, "node", 4, &got) != V_STATUS_SUCCESS || got != &value) {
        return fail("hashkv");
    }
    if (vds_hashkv_del(kv, "node", 4) != V_STATUS_SUCCESS ||
        vds_hashkv_get(kv, "node", 4, &got) != V_STATUS_ITEM_NOT_FOUND) {
        return fail("hashkv del");
    }
    vds_hashkv_destroy(kv);
    return 0;
}

int main(void)
{
    if (test_list() != 0 || test_stack() != 0 || test_queue() != 0 || test_stores() != 0) {
        return 1;
    }
    return 0;
}

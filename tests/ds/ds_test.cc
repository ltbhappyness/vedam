/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
extern "C" {
#include "ds/hashkv.h"
#include "ds/index.h"
#include "ds/list.h"
#include "ds/queue.h"
#include "ds/singleton.h"
#include "ds/stack.h"
}

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>

namespace {

constexpr int kScale = 256;

TEST(DsList, BasicOrderAndDelete)
{
    struct vds_list head;
    struct vds_list first;
    struct vds_list last;

    vds_list_init(&head);
    vds_list_init(&first);
    vds_list_init(&last);
    vds_list_add(&first, &head);
    vds_list_add_tail(&last, &head);
    EXPECT_EQ(vds_list_count(&head), 2u);
    EXPECT_EQ(head.next, &first);
    EXPECT_EQ(head.prev, &last);
    vds_list_del(&first);
    EXPECT_EQ(vds_list_count(&head), 1u);
    EXPECT_EQ(first.next, nullptr);
}

TEST(DsList, EmptyAndSingle)
{
    struct vds_list head;
    struct vds_list only;

    vds_list_init(&head);
    EXPECT_EQ(vds_list_count(&head), 0u);
    EXPECT_EQ(vds_list_empty(&head), 1);
    vds_list_init(&only);
    vds_list_add(&only, &head);
    EXPECT_EQ(vds_list_count(&head), 1u);
    vds_list_del(&only);
    EXPECT_EQ(vds_list_empty(&head), 1);
}

TEST(DsStack, PushPopAndEmpty)
{
    vds_stack_t stack;
    int first = 1;
    int second = 2;
    void *item = nullptr;

    vds_stack_init(&stack);
    EXPECT_EQ(vds_stack_pop(&stack, &item), V_STATUS_ITEM_NOT_FOUND);
    ASSERT_EQ(vds_stack_push(&stack, &first), V_STATUS_SUCCESS);
    ASSERT_EQ(vds_stack_push(&stack, &second), V_STATUS_SUCCESS);
    ASSERT_EQ(vds_stack_pop(&stack, &item), V_STATUS_SUCCESS);
    EXPECT_EQ(item, &second);
    vds_stack_clear(&stack);
    EXPECT_EQ(vds_stack_pop(&stack, &item), V_STATUS_ITEM_NOT_FOUND);
}

TEST(DsStack, NullAndScale)
{
    vds_stack_t stack;
    int items[kScale];
    void *item = nullptr;
    int i;

    EXPECT_EQ(vds_stack_push(nullptr, &i), V_STATUS_INVALID_PARAMETER);
    vds_stack_init(&stack);
    for (i = 0; i < kScale; i++) {
        items[i] = i;
        ASSERT_EQ(vds_stack_push(&stack, &items[i]), V_STATUS_SUCCESS);
    }
    for (i = kScale - 1; i >= 0; i--) {
        ASSERT_EQ(vds_stack_pop(&stack, &item), V_STATUS_SUCCESS);
        EXPECT_EQ(item, &items[i]);
    }
    EXPECT_EQ(vds_stack_pop(&stack, &item), V_STATUS_ITEM_NOT_FOUND);
}

TEST(DsQueue, FifoAndEmpty)
{
    vds_queue_t queue;
    int first = 1;
    int second = 2;
    void *item = nullptr;

    vds_queue_init(&queue);
    EXPECT_EQ(vds_queue_dequeue(&queue, &item), V_STATUS_ITEM_NOT_FOUND);
    ASSERT_EQ(vds_queue_enqueue(&queue, &first), V_STATUS_SUCCESS);
    ASSERT_EQ(vds_queue_enqueue(&queue, &second), V_STATUS_SUCCESS);
    ASSERT_EQ(vds_queue_dequeue(&queue, &item), V_STATUS_SUCCESS);
    EXPECT_EQ(item, &first);
    vds_queue_clear(&queue);
    EXPECT_EQ(vds_queue_dequeue(&queue, &item), V_STATUS_ITEM_NOT_FOUND);
}

TEST(DsQueue, NullAndScale)
{
    vds_queue_t queue;
    int items[kScale];
    void *item = nullptr;
    int i;

    EXPECT_EQ(vds_queue_enqueue(nullptr, &i), V_STATUS_INVALID_PARAMETER);
    vds_queue_init(&queue);
    for (i = 0; i < kScale; i++) {
        items[i] = i;
        ASSERT_EQ(vds_queue_enqueue(&queue, &items[i]), V_STATUS_SUCCESS);
    }
    for (i = 0; i < kScale; i++) {
        ASSERT_EQ(vds_queue_dequeue(&queue, &item), V_STATUS_SUCCESS);
        EXPECT_EQ(item, &items[i]);
    }
}

TEST(DsSingleton, SetGetAndMissing)
{
    vds_singleton_t *store = nullptr;
    int value = 7;
    void *got = nullptr;

    EXPECT_EQ(vds_singleton_create(nullptr), V_STATUS_INVALID_PARAMETER);
    ASSERT_EQ(vds_singleton_create(&store), V_STATUS_SUCCESS);
    EXPECT_EQ(vds_singleton_get(store, &got), V_STATUS_ITEM_NOT_FOUND);
    ASSERT_EQ(vds_singleton_set(store, &value), V_STATUS_SUCCESS);
    ASSERT_EQ(vds_singleton_get(store, &got), V_STATUS_SUCCESS);
    EXPECT_EQ(got, &value);
    vds_singleton_destroy(store);
}

TEST(DsIndex, GrowPastInitialCapacity)
{
    vds_index_t *index = nullptr;
    int value = 9;
    void *got = nullptr;

    EXPECT_EQ(vds_index_create(nullptr, 1), V_STATUS_INVALID_PARAMETER);
    ASSERT_EQ(vds_index_create(&index, 1), V_STATUS_SUCCESS);
    EXPECT_EQ(vds_index_get(index, 0, &got), V_STATUS_ITEM_NOT_FOUND);
    ASSERT_EQ(vds_index_set(index, 5, &value), V_STATUS_SUCCESS);
    ASSERT_EQ(vds_index_get(index, 5, &got), V_STATUS_SUCCESS);
    EXPECT_EQ(got, &value);
    ASSERT_EQ(vds_index_del(index, 5), V_STATUS_SUCCESS);
    EXPECT_EQ(vds_index_get(index, 5, &got), V_STATUS_ITEM_NOT_FOUND);
    vds_index_destroy(index);
}

TEST(DsHashKv, ReplaceDeleteAndScale)
{
    vds_hashkv_t *store = nullptr;
    int values[kScale];
    char key[16];
    void *got = nullptr;
    int i;

    EXPECT_EQ(vds_hashkv_create(nullptr), V_STATUS_INVALID_PARAMETER);
    ASSERT_EQ(vds_hashkv_create(&store), V_STATUS_SUCCESS);
    EXPECT_EQ(vds_hashkv_get(store, "missing", 7, &got), V_STATUS_ITEM_NOT_FOUND);
    values[0] = 1;
    ASSERT_EQ(vds_hashkv_set(store, "node", 4, &values[0]), V_STATUS_SUCCESS);
    values[0] = 2;
    ASSERT_EQ(vds_hashkv_set(store, "node", 4, &values[0]), V_STATUS_SUCCESS);
    ASSERT_EQ(vds_hashkv_get(store, "node", 4, &got), V_STATUS_SUCCESS);
    EXPECT_EQ(got, &values[0]);
    ASSERT_EQ(vds_hashkv_del(store, "node", 4), V_STATUS_SUCCESS);
    EXPECT_EQ(vds_hashkv_get(store, "node", 4, &got), V_STATUS_ITEM_NOT_FOUND);

    for (i = 0; i < kScale; i++) {
        values[i] = i;
        ASSERT_GT(snprintf(key, sizeof(key), "k%d", i), 0);
        ASSERT_EQ(vds_hashkv_set(store, key, static_cast<vuint32_t>(strlen(key)), &values[i]),
                  V_STATUS_SUCCESS);
    }
    for (i = 0; i < kScale; i++) {
        ASSERT_GT(snprintf(key, sizeof(key), "k%d", i), 0);
        ASSERT_EQ(vds_hashkv_get(store, key, static_cast<vuint32_t>(strlen(key)), &got),
                  V_STATUS_SUCCESS);
        EXPECT_EQ(got, &values[i]);
    }
    vds_hashkv_destroy(store);
}

} /* namespace */

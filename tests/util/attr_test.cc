/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
extern "C" {
#include "util/attr.h"
}

#include <gtest/gtest.h>

#include <cstring>

/**
 * @brief Point @p attr at @p buf as text with @p room bytes of room.
 *
 * @param[out] attr Attribute to prepare
 * @param[in]  buf  Caller buffer
 * @param[in]  room Bytes of room in @p buf
 */
static void text_room(v_attr_t *attr, char *buf, vuint32_t room)
{
    std::memset(attr, 0, sizeof(*attr));
    attr->value.text.buf = buf;
    attr->value.text.len = room;
}

/**
 * @brief Point @p attr at @p items as a list with @p room items of room.
 *
 * @param[out] attr  Attribute to prepare
 * @param[in]  items Caller array
 * @param[in]  room  Items of room in @p items
 */
static void list_room(v_attr_t *attr, void *items, vuint32_t room)
{
    std::memset(attr, 0, sizeof(*attr));
    attr->value.list.items = items;
    attr->value.list.count = room;
}

/**
 * @brief Drop a write watch left on this thread.
 */
static void watch_clear(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        vattr_write_unwatch();
    }
}

TEST(AttrPutText, Basic)
{
    char buf[16];
    v_attr_t attr;

    text_room(&attr, buf, sizeof(buf));
    EXPECT_EQ(vattr_put_text(&attr, "abc", 3), V_STATUS_SUCCESS);
    EXPECT_EQ(attr.value.text.len, 3u);
    EXPECT_EQ(std::memcmp(buf, "abc", 3), 0);
}

TEST(AttrPutText, Bounds)
{
    char buf[8];
    v_attr_t attr;

    text_room(&attr, buf, 4);
    EXPECT_EQ(vattr_put_text(&attr, "abcd", 4), V_STATUS_SUCCESS);
    EXPECT_EQ(attr.value.text.len, 4u);
    EXPECT_EQ(std::memcmp(buf, "abcd", 4), 0);

    std::memset(buf, 'x', sizeof(buf));
    text_room(&attr, buf, 4);
    EXPECT_EQ(vattr_put_text(&attr, "abcde", 5), V_STATUS_BUFFER_OVERFLOW);
    EXPECT_EQ(attr.value.text.len, 5u);
    for (char c : buf) {
        EXPECT_EQ(c, 'x');
    }
}

TEST(AttrPutText, Empty)
{
    char buf[4] = {'x', 'x', 'x', 'x'};
    v_attr_t attr;

    text_room(&attr, buf, sizeof(buf));
    EXPECT_EQ(vattr_put_text(&attr, nullptr, 0), V_STATUS_SUCCESS);
    EXPECT_EQ(attr.value.text.len, 0u);
    EXPECT_EQ(buf[0], 'x');

    text_room(&attr, nullptr, 0);
    EXPECT_EQ(vattr_put_text(&attr, "", 0), V_STATUS_SUCCESS);
    EXPECT_EQ(attr.value.text.len, 0u);
}

TEST(AttrPutText, Negative)
{
    char buf[4];
    v_attr_t attr;

    EXPECT_EQ(vattr_put_text(nullptr, "a", 1), V_STATUS_INVALID_PARAMETER);
    text_room(&attr, buf, sizeof(buf));
    EXPECT_EQ(vattr_put_text(&attr, nullptr, 1), V_STATUS_INVALID_PARAMETER);
    EXPECT_EQ(attr.value.text.len, sizeof(buf));
}

TEST(AttrPutList, Basic)
{
    const vuint32_t src[3] = {7, 8, 9};
    vuint32_t dst[4] = {0, 0, 0, 0};
    v_attr_t attr;

    list_room(&attr, dst, 4);
    EXPECT_EQ(vattr_put_list(&attr, src, 3, sizeof(src[0])), V_STATUS_SUCCESS);
    EXPECT_EQ(attr.value.list.count, 3u);
    EXPECT_EQ(dst[0], 7u);
    EXPECT_EQ(dst[2], 9u);
    EXPECT_EQ(dst[3], 0u);
}

TEST(AttrPutList, Bounds)
{
    const vuint64_t src[3] = {1, 2, 3};
    vuint64_t dst[3] = {5, 5, 5};
    v_attr_t attr;

    list_room(&attr, dst, 2);
    EXPECT_EQ(vattr_put_list(&attr, src, 2, sizeof(src[0])), V_STATUS_SUCCESS);
    EXPECT_EQ(attr.value.list.count, 2u);
    EXPECT_EQ(dst[1], 2u);
    EXPECT_EQ(dst[2], 5u);

    dst[0] = 5;
    dst[1] = 5;
    list_room(&attr, dst, 2);
    EXPECT_EQ(vattr_put_list(&attr, src, 3, sizeof(src[0])), V_STATUS_BUFFER_OVERFLOW);
    EXPECT_EQ(attr.value.list.count, 3u);
    for (vuint64_t v : dst) {
        EXPECT_EQ(v, 5u);
    }
}

TEST(AttrPutList, Empty)
{
    vuint32_t dst[2] = {5, 5};
    v_attr_t attr;

    list_room(&attr, dst, 2);
    EXPECT_EQ(vattr_put_list(&attr, nullptr, 0, sizeof(dst[0])), V_STATUS_SUCCESS);
    EXPECT_EQ(attr.value.list.count, 0u);
    EXPECT_EQ(dst[0], 5u);

    list_room(&attr, nullptr, 0);
    EXPECT_EQ(vattr_put_list(&attr, dst, 0, sizeof(dst[0])), V_STATUS_SUCCESS);
    EXPECT_EQ(attr.value.list.count, 0u);
}

TEST(AttrPutList, Negative)
{
    vuint32_t dst[2];
    v_attr_t attr;

    EXPECT_EQ(vattr_put_list(nullptr, dst, 1, sizeof(dst[0])), V_STATUS_INVALID_PARAMETER);
    list_room(&attr, dst, 2);
    EXPECT_EQ(vattr_put_list(&attr, nullptr, 1, sizeof(dst[0])), V_STATUS_INVALID_PARAMETER);
    EXPECT_EQ(attr.value.list.count, 2u);
}

TEST(AttrWatch, Basic)
{
    v_attr_t attrs[2];
    v_attr_t outside;
    char buf[8];
    char other[4];

    watch_clear();
    EXPECT_EQ(vattr_was_written(&attrs[0]), 0);
    EXPECT_EQ(vattr_was_written(nullptr), 0);

    text_room(&attrs[0], buf, sizeof(buf));
    vattr_write_watch(nullptr, 2);
    EXPECT_EQ(vattr_put_text(&attrs[0], "ab", 2), V_STATUS_SUCCESS);
    EXPECT_EQ(vattr_was_written(&attrs[0]), 0);
    vattr_write_unwatch();

    vattr_write_watch(attrs, 2);
    text_room(&attrs[0], buf, sizeof(buf));
    EXPECT_EQ(vattr_put_text(&attrs[0], nullptr, 0), V_STATUS_SUCCESS);
    EXPECT_EQ(vattr_was_written(&attrs[0]), 1);
    EXPECT_EQ(vattr_was_written(&attrs[1]), 0);

    text_room(&outside, other, sizeof(other));
    EXPECT_EQ(vattr_put_text(&outside, "zz", 2), V_STATUS_SUCCESS);
    EXPECT_EQ(vattr_was_written(&outside), 0);
    EXPECT_EQ(vattr_was_written(&attrs[0]), 1);

    vattr_write_unwatch();
    EXPECT_EQ(vattr_was_written(&attrs[0]), 0);
    vattr_write_unwatch();
    EXPECT_EQ(vattr_was_written(&attrs[0]), 0);
}

TEST(AttrWatch, Negative)
{
    v_attr_t attr;
    char buf[4] = {'x', 'x', 'x', 'x'};

    watch_clear();
    vattr_write_watch(&attr, 1);
    EXPECT_EQ(vattr_put_text(nullptr, "a", 1), V_STATUS_INVALID_PARAMETER);
    EXPECT_EQ(vattr_was_written(nullptr), 0);
    text_room(&attr, buf, sizeof(buf));
    EXPECT_EQ(vattr_put_text(&attr, nullptr, 1), V_STATUS_INVALID_PARAMETER);
    EXPECT_EQ(vattr_was_written(&attr), 0);
    EXPECT_EQ(attr.value.text.len, sizeof(buf));
    EXPECT_EQ(buf[0], 'x');
    vattr_write_unwatch();
    EXPECT_EQ(vattr_was_written(&attr), 0);
}

TEST(AttrWatch, BoundsFullRoom)
{
    char src[256];
    char buf[256];
    v_attr_t attrs[2];
    const vuint32_t more[3] = {1, 2, 3};
    vuint32_t dst[2] = {9, 9};
    unsigned i;

    watch_clear();
    for (i = 0; i < sizeof(src); i++) {
        src[i] = static_cast<char>(1 + (i % 251));
    }
    std::memset(buf, 0, sizeof(buf));
    vattr_write_watch(attrs, 2);
    text_room(&attrs[0], buf, sizeof(buf));
    EXPECT_EQ(vattr_put_text(&attrs[0], src, sizeof(src)), V_STATUS_SUCCESS);
    EXPECT_EQ(attrs[0].value.text.len, sizeof(src));
    EXPECT_EQ(vattr_was_written(&attrs[0]), 1);
    EXPECT_EQ(std::memcmp(buf, src, sizeof(src)), 0);

    std::memset(buf, 'x', 4);
    text_room(&attrs[0], buf, 4);
    EXPECT_EQ(vattr_put_text(&attrs[0], "abcde", 5), V_STATUS_BUFFER_OVERFLOW);
    EXPECT_EQ(attrs[0].value.text.len, 5u);
    EXPECT_EQ(vattr_was_written(&attrs[0]), 1);
    EXPECT_EQ(buf[0], 'x');

    list_room(&attrs[1], dst, 2);
    EXPECT_EQ(vattr_put_list(&attrs[1], more, 2, sizeof(more[0])), V_STATUS_SUCCESS);
    EXPECT_EQ(attrs[1].value.list.count, 2u);
    EXPECT_EQ(vattr_was_written(&attrs[1]), 1);
    EXPECT_EQ(dst[0], 1u);
    EXPECT_EQ(dst[1], 2u);

    dst[0] = 9;
    dst[1] = 9;
    list_room(&attrs[1], dst, 2);
    EXPECT_EQ(vattr_put_list(&attrs[1], more, 3, sizeof(more[0])), V_STATUS_BUFFER_OVERFLOW);
    EXPECT_EQ(attrs[1].value.list.count, 3u);
    EXPECT_EQ(vattr_was_written(&attrs[1]), 1);
    EXPECT_EQ(dst[0], 9u);
    EXPECT_EQ(dst[1], 9u);
    vattr_write_unwatch();
    EXPECT_EQ(vattr_was_written(&attrs[0]), 0);
    EXPECT_EQ(vattr_was_written(&attrs[1]), 0);
}

TEST(AttrWatch, CornerNesting)
{
    v_attr_t level[4];
    v_attr_t outside;
    char buf[4][4];
    char extra[4];
    char src[4] = {'w', 'x', 'y', 'z'};
    int i;

    watch_clear();
    text_room(&level[0], buf[0], 4);
    vattr_write_watch(&level[0], 1);
    EXPECT_EQ(vattr_put_text(&level[0], src, 4), V_STATUS_SUCCESS);
    EXPECT_EQ(vattr_was_written(&level[0]), 1);

    vattr_write_watch(nullptr, 1);
    text_room(&outside, extra, 4);
    EXPECT_EQ(vattr_put_text(&outside, src, 4), V_STATUS_SUCCESS);
    EXPECT_EQ(vattr_was_written(&outside), 0);
    EXPECT_EQ(vattr_was_written(&level[0]), 0);
    vattr_write_unwatch();
    EXPECT_EQ(vattr_was_written(&level[0]), 1);

    for (i = 1; i < 4; i++) {
        text_room(&level[i], buf[i], 4);
        vattr_write_watch(&level[i], 1);
    }
    EXPECT_EQ(vattr_was_written(&level[0]), 0);
    EXPECT_EQ(vattr_put_text(&level[3], src, 4), V_STATUS_SUCCESS);
    EXPECT_EQ(vattr_was_written(&level[3]), 1);

    vattr_write_watch(&outside, 1);
    vattr_write_unwatch();
    EXPECT_EQ(vattr_was_written(&level[3]), 1);

    vattr_write_unwatch();
    vattr_write_unwatch();
    vattr_write_unwatch();
    EXPECT_EQ(vattr_was_written(&level[0]), 1);
    EXPECT_EQ(vattr_was_written(&level[3]), 0);
    EXPECT_EQ(std::memcmp(buf[0], src, 4), 0);
    vattr_write_unwatch();
    EXPECT_EQ(vattr_was_written(&level[0]), 0);
}

TEST(AttrCollapse, LastWinsAndEmpty)
{
    v_attr_t attrs[4];
    vuint32_t count;

    EXPECT_EQ(vattr_collapse_last(nullptr, 4), 0u);
    EXPECT_EQ(vattr_collapse_last(attrs, 0), 0u);

    attrs[0].id = 1;
    attrs[0].value.s32 = 1;
    attrs[1].id = 2;
    attrs[2].id = 1;
    attrs[2].value.s32 = 9;
    attrs[3].id = 3;
    count = vattr_collapse_last(attrs, 4);
    ASSERT_EQ(count, 3u);
    EXPECT_EQ(attrs[0].id, 2u);
    EXPECT_EQ(attrs[1].id, 1u);
    EXPECT_EQ(attrs[1].value.s32, 9);
    EXPECT_EQ(attrs[2].id, 3u);
}

TEST(AttrDup, CopyFreeAndZero)
{
    v_attr_t src;
    v_attr_t *copy;

    src.id = 4;
    src.value.s32 = 11;
    EXPECT_EQ(vattr_dup(nullptr, 0), nullptr);
    EXPECT_EQ(vattr_dup(&src, 0), nullptr);
    copy = vattr_dup(&src, 1);
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->id, src.id);
    EXPECT_EQ(copy->value.s32, src.value.s32);
    vattr_free(copy);
    vattr_free(nullptr);
}

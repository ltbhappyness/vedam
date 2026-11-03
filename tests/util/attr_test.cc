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

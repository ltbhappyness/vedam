/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "vedam/types.pb.h"

#include <gtest/gtest.h>

#include <string>

TEST(ProtoMessage, EmptyRoundTrip)
{
    vedam::Message in;
    vedam::Message out;
    std::string bytes;

    ASSERT_TRUE(in.SerializeToString(&bytes));
    ASSERT_TRUE(out.ParseFromString(bytes));
    EXPECT_EQ(out.attrs_size(), 0);
}

TEST(ProtoMessage, RejectsGarbage)
{
    vedam::Message out;

    EXPECT_FALSE(out.ParseFromString(std::string("\xff\xff", 2)));
}

TEST(ProtoMessage, ManyAttributes)
{
    vedam::Message in;
    vedam::Message out;
    std::string bytes;
    int i;

    for (i = 0; i < 64; i++) {
        in.add_attrs()->set_id(static_cast<uint32_t>(i + 1));
    }
    ASSERT_TRUE(in.SerializeToString(&bytes));
    ASSERT_TRUE(out.ParseFromString(bytes));
    ASSERT_EQ(out.attrs_size(), 64);
    EXPECT_EQ(out.attrs(0).id(), 1u);
    EXPECT_EQ(out.attrs(63).id(), 64u);
}

/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
extern "C" {
#include "platform/memory.h"
}

#include <gtest/gtest.h>

TEST(PlatformMemory, ZeroedBlockAndFreeNull)
{
    auto *block = static_cast<unsigned char *>(vplatform_calloc(8, 1));
    int i;

    ASSERT_NE(block, nullptr);
    for (i = 0; i < 8; i++) {
        EXPECT_EQ(block[i], 0);
    }
    vplatform_free(block);
    vplatform_free(nullptr);
}

TEST(PlatformMemory, MemsetMemcpyAndHugeProduct)
{
    unsigned char src[4] = {1, 2, 3, 4};
    unsigned char dst[4] = {0, 0, 0, 0};

    vplatform_memset(dst, 0xab, sizeof(dst));
    EXPECT_EQ(dst[0], 0xab);
    EXPECT_EQ(dst[3], 0xab);
    vplatform_memcpy(dst, src, sizeof(src));
    EXPECT_EQ(dst[0], 1);
    EXPECT_EQ(dst[3], 4);
    EXPECT_EQ(vplatform_calloc(static_cast<size_t>(-1), static_cast<size_t>(-1)), nullptr);
}

TEST(PlatformMemory, Scale)
{
    constexpr int kCount = 4096;
    auto *block = static_cast<int *>(vplatform_calloc(static_cast<size_t>(kCount), sizeof(int)));
    int i;

    ASSERT_NE(block, nullptr);
    for (i = 0; i < kCount; i++) {
        EXPECT_EQ(block[i], 0);
        block[i] = i;
    }
    EXPECT_EQ(block[0], 0);
    EXPECT_EQ(block[kCount - 1], kCount - 1);
    vplatform_free(block);
}

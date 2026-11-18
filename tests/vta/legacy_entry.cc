/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
extern "C" {
#include "vti/vti.h"
}

#include <gtest/gtest.h>

extern "C" int vedam_legacy_codec(void);
extern "C" int vedam_vta_values(int link);
extern "C" int vedam_vta_text_bounds(int link);
extern "C" int vedam_vta_list(int link);
extern "C" int vedam_vta_empty_commit(int link);
extern "C" int vedam_vta_full_text(int link);

TEST(VtaCodecLegacy, ExistingCases)
{
    EXPECT_EQ(vedam_legacy_codec(), 0);
}

TEST(VtaValues, BasicStructAndTextOverMq)
{
#if !VEDAM_ENABLE_MQ
    GTEST_SKIP() << "mq disabled in this defconfig";
#else
    EXPECT_EQ(vedam_vta_values(VTI_LINK_IPC), 0);
#endif
}

TEST(VtaValues, BasicStructAndTextOverHttp)
{
#if !VEDAM_ENABLE_GRPC
    GTEST_SKIP() << "grpc disabled in this defconfig";
#else
    EXPECT_EQ(vedam_vta_values(VTI_LINK_REMOTE), 0);
#endif
}

TEST(VtaValues, BoundsTextOverMq)
{
#if !VEDAM_ENABLE_MQ
    GTEST_SKIP() << "mq disabled in this defconfig";
#else
    EXPECT_EQ(vedam_vta_text_bounds(VTI_LINK_IPC), 0);
#endif
}

TEST(VtaValues, BoundsTextOverHttp)
{
#if !VEDAM_ENABLE_GRPC
    GTEST_SKIP() << "grpc disabled in this defconfig";
#else
    EXPECT_EQ(vedam_vta_text_bounds(VTI_LINK_REMOTE), 0);
#endif
}

TEST(VtaValues, BoundsListOverMq)
{
#if !VEDAM_ENABLE_MQ
    GTEST_SKIP() << "mq disabled in this defconfig";
#else
    EXPECT_EQ(vedam_vta_list(VTI_LINK_IPC), 0);
#endif
}

TEST(VtaValues, BoundsListOverHttp)
{
#if !VEDAM_ENABLE_GRPC
    GTEST_SKIP() << "grpc disabled in this defconfig";
#else
    EXPECT_EQ(vedam_vta_list(VTI_LINK_REMOTE), 0);
#endif
}

TEST(VtaValues, EmptyCommitOverMq)
{
#if !VEDAM_ENABLE_MQ
    GTEST_SKIP() << "mq disabled in this defconfig";
#else
    EXPECT_EQ(vedam_vta_empty_commit(VTI_LINK_IPC), 0);
#endif
}

TEST(VtaValues, EmptyCommitOverHttp)
{
#if !VEDAM_ENABLE_GRPC
    GTEST_SKIP() << "grpc disabled in this defconfig";
#else
    EXPECT_EQ(vedam_vta_empty_commit(VTI_LINK_REMOTE), 0);
#endif
}

TEST(VtaValues, BoundsFullTextOverMq)
{
#if !VEDAM_ENABLE_MQ
    GTEST_SKIP() << "mq disabled in this defconfig";
#else
    EXPECT_EQ(vedam_vta_full_text(VTI_LINK_IPC), 0);
#endif
}

TEST(VtaValues, BoundsFullTextOverHttp)
{
#if !VEDAM_ENABLE_GRPC
    GTEST_SKIP() << "grpc disabled in this defconfig";
#else
    EXPECT_EQ(vedam_vta_full_text(VTI_LINK_REMOTE), 0);
#endif
}

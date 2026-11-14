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
    EXPECT_EQ(vedam_vta_values(VTI_LINK_IPC), 0);
}

TEST(VtaValues, BasicStructAndTextOverHttp)
{
    EXPECT_EQ(vedam_vta_values(VTI_LINK_REMOTE), 0);
}

TEST(VtaValues, BoundsTextOverMq)
{
    EXPECT_EQ(vedam_vta_text_bounds(VTI_LINK_IPC), 0);
}

TEST(VtaValues, BoundsTextOverHttp)
{
    EXPECT_EQ(vedam_vta_text_bounds(VTI_LINK_REMOTE), 0);
}

TEST(VtaValues, BoundsListOverMq)
{
    EXPECT_EQ(vedam_vta_list(VTI_LINK_IPC), 0);
}

TEST(VtaValues, BoundsListOverHttp)
{
    EXPECT_EQ(vedam_vta_list(VTI_LINK_REMOTE), 0);
}

TEST(VtaValues, EmptyCommitOverMq)
{
    EXPECT_EQ(vedam_vta_empty_commit(VTI_LINK_IPC), 0);
}

TEST(VtaValues, EmptyCommitOverHttp)
{
    EXPECT_EQ(vedam_vta_empty_commit(VTI_LINK_REMOTE), 0);
}

TEST(VtaValues, BoundsFullTextOverMq)
{
    EXPECT_EQ(vedam_vta_full_text(VTI_LINK_IPC), 0);
}

TEST(VtaValues, BoundsFullTextOverHttp)
{
    EXPECT_EQ(vedam_vta_full_text(VTI_LINK_REMOTE), 0);
}

/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include <gtest/gtest.h>

extern "C" int vedam_legacy_codec(void);

TEST(VtaCodecLegacy, ExistingCases)
{
    EXPECT_EQ(vedam_legacy_codec(), 0);
}

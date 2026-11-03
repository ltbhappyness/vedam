/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "vi/class/system.h"
#include "vi/vedam.h"

#include <gtest/gtest.h>

TEST(SystemApi, GetBeforeInit)
{
    vi_msg_t msg = {};

    EXPECT_EQ(vi_system_get(&msg), V_STATUS_UNINITIALIZED);
}

TEST(SystemApi, NullAndEmptyGet)
{
    vi_msg_t msg = {};

    ASSERT_EQ(vedam_init(), V_STATUS_SUCCESS);
    EXPECT_EQ(vi_system_get(nullptr), V_STATUS_INVALID_PARAMETER);
    EXPECT_EQ(vi_system_get(&msg), V_STATUS_SUCCESS);
    EXPECT_EQ(vedam_deinit(), V_STATUS_SUCCESS);
}

TEST(SystemApi, OneAttribute)
{
    vi_msg_t msg = {};
    v_attr_t attr = {};
    v_status_t status = V_STATUS_FAILURE;

    attr.id = VI_SYSTEM_ATTR_NODE_CLASS;
    msg.attr_list = &attr;
    msg.status_list = &status;
    msg.attr_count = 1;
    ASSERT_EQ(vedam_init(), V_STATUS_SUCCESS);
    EXPECT_EQ(vi_system_get(&msg), V_STATUS_SUCCESS);
    EXPECT_EQ(vedam_deinit(), V_STATUS_SUCCESS);
}

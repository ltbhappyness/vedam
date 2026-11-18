/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
extern "C" {
#include "core/node.h"
#include "lock/store.h"
#include "platform/memory.h"
#include "vii/class/system.h"
#include "vii/dispatch.h"
#include "vii/meta.h"
}

#include <gtest/gtest.h>

#include <cstring>

namespace
{

constexpr vuint32_t kScale = 32;

/**
 * @brief Return the System row of the class table.
 *
 * @return The System class row.
 */
const struct vii_class *system_class()
{
    return &vii_classes[V_CLASS_SYSTEM];
}

/**
 * @brief Build an internal call to System for @p verb from @p origin.
 *
 * @param[in] origin Call origin
 * @param[in] verb   Verb
 * @param[in] msg    Call message
 * @return The call.
 */
vii_msg_t system_call(v_origin_t origin, v_verb_t verb, vi_msg_t *msg)
{
    vii_msg_t call;

    std::memset(&call, 0, sizeof(call));
    call.class_id = V_CLASS_SYSTEM;
    call.origin = origin;
    call.verb = verb;
    call.major = VII_SYSTEM_MAJOR;
    call.minor = VII_SYSTEM_MINOR;
    call.msg = msg;
    return call;
}

class Via : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        vedam_t *node = static_cast<vedam_t *>(vplatform_calloc(1, sizeof(*node)));

        ASSERT_NE(node, nullptr);
        node->classes = vii_classes;
        ASSERT_EQ(vedam_lockdb_open(node), V_STATUS_SUCCESS);
        g_vedam_node = node;
    }

    void TearDown() override
    {
        vedam_t *node = g_vedam_node;

        g_vedam_node = nullptr;
        vedam_lockdb_close(node);
        vplatform_free(node);
    }
};

TEST_F(Via, AttrCheckBasicGetEverySystemAttribute)
{
    vi_system_state_t state = {};
    v_attr_t attrs[4] = {};
    v_status_t statuses[4];
    vi_msg_t msg = {};

    attrs[0].id = VII_SYSTEM_ATTR_NODE_CLASS;
    attrs[1].id = VII_SYSTEM_ATTR_PROTOCOL_VERSION;
    attrs[2].id = VII_SYSTEM_ATTR_SYSTEM_STATE;
    attrs[2].value.ptr = &state;
    attrs[3].id = VII_SYSTEM_ATTR_RELEASE_COMMIT;
    msg.attr_count = 4;
    msg.attr_list = attrs;
    msg.status_list = statuses;
    EXPECT_EQ(vii_attr_check(system_class(), V_ORIGIN_EXTERNAL, &msg, V_VERB_GET),
              V_STATUS_SUCCESS);
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(statuses[i], V_STATUS_SUCCESS);
    }
}

TEST_F(Via, AttrCheckBasicInternalSetOfKnownEnumValue)
{
    v_attr_t attr = {};
    v_status_t status;
    vi_msg_t msg = {};

    attr.id = VII_SYSTEM_ATTR_NODE_CLASS;
    attr.value.s32 = VI_SYSTEM_NODE_CLASS_C;
    msg.attr_count = 1;
    msg.attr_list = &attr;
    msg.status_list = &status;
    EXPECT_EQ(vii_attr_check(system_class(), V_ORIGIN_INTERNAL, &msg, V_VERB_SET),
              V_STATUS_SUCCESS);
}

TEST_F(Via, AttrCheckNegativeUnknownId)
{
    v_attr_t attrs[2] = {};
    v_status_t statuses[2];
    vi_msg_t msg = {};
    v_status_t unknown = static_cast<v_status_t>(V_STATUS_UNKNOWN_ATTRIBUTE_0 - 1);

    attrs[0].id = VII_SYSTEM_ATTR_NODE_CLASS;
    attrs[1].id = 99;
    msg.attr_count = 2;
    msg.attr_list = attrs;
    msg.status_list = statuses;
    EXPECT_EQ(vii_attr_check(system_class(), V_ORIGIN_EXTERNAL, &msg, V_VERB_GET), unknown);
    EXPECT_EQ(statuses[0], V_STATUS_SUCCESS);
    EXPECT_EQ(statuses[1], unknown);
}

TEST_F(Via, AttrCheckNegativeReadOnlyOnExternalSet)
{
    v_attr_t attr = {};
    v_status_t status;
    vi_msg_t msg = {};

    attr.id = VII_SYSTEM_ATTR_NODE_CLASS;
    attr.value.s32 = VI_SYSTEM_NODE_CLASS_A;
    msg.attr_count = 1;
    msg.attr_list = &attr;
    msg.status_list = &status;
    EXPECT_EQ(vii_attr_check(system_class(), V_ORIGIN_EXTERNAL, &msg, V_VERB_SET),
              V_STATUS_ATTR_NOT_SUPPORTED_0);
    EXPECT_EQ(status, V_STATUS_ATTR_NOT_SUPPORTED_0);
}

TEST_F(Via, AttrCheckBoundsEnumMembershipOnInternalSet)
{
    v_attr_t attr = {};
    v_status_t status;
    vi_msg_t msg = {};

    attr.id = VII_SYSTEM_ATTR_NODE_CLASS;
    attr.value.s32 = 99;
    msg.attr_count = 1;
    msg.attr_list = &attr;
    msg.status_list = &status;
    EXPECT_EQ(vii_attr_check(system_class(), V_ORIGIN_INTERNAL, &msg, V_VERB_SET),
              V_STATUS_INVALID_ATTR_VALUE_0);
    EXPECT_EQ(status, V_STATUS_INVALID_ATTR_VALUE_0);
}

TEST_F(Via, AttrCheckScaleThirtyTwoAttributes)
{
    static const v_attr_id_t ids[] = {VII_SYSTEM_ATTR_NODE_CLASS, VII_SYSTEM_ATTR_PROTOCOL_VERSION,
                                      VII_SYSTEM_ATTR_RELEASE_COMMIT};
    v_attr_t attrs[kScale] = {};
    v_status_t statuses[kScale];
    vi_msg_t msg = {};

    for (vuint32_t i = 0; i < kScale; i++) {
        attrs[i].id = ids[i % 3];
    }
    msg.attr_count = kScale;
    msg.attr_list = attrs;
    msg.status_list = statuses;
    EXPECT_EQ(vii_attr_check(system_class(), V_ORIGIN_EXTERNAL, &msg, V_VERB_GET),
              V_STATUS_SUCCESS);
    for (vuint32_t i = 0; i < kScale; i++) {
        EXPECT_EQ(statuses[i], V_STATUS_SUCCESS);
    }
}

TEST_F(Via, VerbNegativeInternalVerbFromExternalOrigin)
{
    vi_msg_t msg = {};
    vii_msg_t call = system_call(V_ORIGIN_EXTERNAL, V_VERB_SET, &msg);

    EXPECT_EQ(vii_verb(system_class(), &call), V_STATUS_NOT_IMPLEMENTED);
}

TEST_F(Via, VerbEmptyList)
{
    vi_msg_t msg = {};
    vii_msg_t call = system_call(V_ORIGIN_EXTERNAL, V_VERB_GET, &msg);

    EXPECT_EQ(vii_verb(system_class(), &call), V_STATUS_SUCCESS);
}

TEST_F(Via, VerbNegativeCountWithoutLists)
{
    vi_msg_t msg = {};
    vii_msg_t call = system_call(V_ORIGIN_EXTERNAL, V_VERB_GET, &msg);

    msg.attr_count = 1;
    EXPECT_EQ(vii_verb(system_class(), &call), V_STATUS_INVALID_PARAMETER);
}

TEST_F(Via, VerbCornerDuplicateIdsKeepTheLast)
{
    v_attr_t attrs[3] = {};
    v_status_t statuses[3];
    vi_msg_t msg = {};
    vii_msg_t call = system_call(V_ORIGIN_EXTERNAL, V_VERB_GET, &msg);

    attrs[0].id = VII_SYSTEM_ATTR_NODE_CLASS;
    attrs[0].value.s32 = 1;
    attrs[1].id = VII_SYSTEM_ATTR_PROTOCOL_VERSION;
    attrs[2].id = VII_SYSTEM_ATTR_NODE_CLASS;
    attrs[2].value.s32 = 3;
    msg.attr_count = 3;
    msg.attr_list = attrs;
    msg.status_list = statuses;
    EXPECT_EQ(vii_verb(system_class(), &call), V_STATUS_SUCCESS);
    ASSERT_EQ(msg.attr_count, 2u);
    EXPECT_EQ(attrs[0].id, static_cast<v_attr_id_t>(VII_SYSTEM_ATTR_PROTOCOL_VERSION));
    EXPECT_EQ(attrs[1].id, static_cast<v_attr_id_t>(VII_SYSTEM_ATTR_NODE_CLASS));
    EXPECT_EQ(attrs[1].value.s32, 3);
}

TEST_F(Via, DispatchBasicGet)
{
    v_attr_t attr = {};
    v_status_t status = V_STATUS_FAILURE;
    vi_msg_t msg = {};
    vii_msg_t call = system_call(V_ORIGIN_EXTERNAL, V_VERB_GET, &msg);

    attr.id = VII_SYSTEM_ATTR_NODE_CLASS;
    msg.attr_count = 1;
    msg.attr_list = &attr;
    msg.status_list = &status;
    EXPECT_EQ(vii_dispatch(&call), V_STATUS_SUCCESS);
    EXPECT_EQ(status, V_STATUS_SUCCESS);
}

TEST_F(Via, DispatchNegativeUninitialized)
{
    vi_msg_t msg = {};
    vii_msg_t call = system_call(V_ORIGIN_EXTERNAL, V_VERB_GET, &msg);
    vedam_t *node = g_vedam_node;

    g_vedam_node = nullptr;
    EXPECT_EQ(vii_dispatch(&call), V_STATUS_UNINITIALIZED);
    g_vedam_node = node;
}

TEST_F(Via, DispatchNegativeNullMessage)
{
    vii_msg_t call = system_call(V_ORIGIN_EXTERNAL, V_VERB_GET, nullptr);

    EXPECT_EQ(vii_dispatch(nullptr), V_STATUS_INVALID_PARAMETER);
    EXPECT_EQ(vii_dispatch(&call), V_STATUS_INVALID_PARAMETER);
}

TEST_F(Via, DispatchNegativeClassOutOfRangeIsRefusedByTheLock)
{
#if !VEDAM_ENABLE_WWLOCK
    GTEST_SKIP() << "wwlock disabled in this defconfig";
#else
    vi_msg_t msg = {};
    vii_msg_t call = system_call(V_ORIGIN_EXTERNAL, V_VERB_GET, &msg);

    call.class_id = V_CLASS_UNSPECIFIED;
    EXPECT_EQ(vii_dispatch(&call), V_STATUS_INVALID_PARAMETER);
    call.class_id = static_cast<v_class_id_t>(V_CLASS_ID_MAX + 1);
    EXPECT_EQ(vii_dispatch(&call), V_STATUS_INVALID_PARAMETER);
#endif
}

TEST_F(Via, DispatchCornerClassOutOfRangeWithoutLockMap)
{
    vi_msg_t msg = {};
    vii_msg_t call = system_call(V_ORIGIN_EXTERNAL, V_VERB_GET, &msg);
    struct vedam_lockdb *locks = g_vedam_node->locks;

    g_vedam_node->locks = nullptr;
    call.class_id = V_CLASS_UNSPECIFIED;
    EXPECT_EQ(vii_dispatch(&call), V_STATUS_NOT_IMPLEMENTED);
    call.class_id = static_cast<v_class_id_t>(V_CLASS_ID_MAX + 1);
    EXPECT_EQ(vii_dispatch(&call), V_STATUS_NOT_IMPLEMENTED);
    g_vedam_node->locks = locks;
}

TEST_F(Via, DispatchBoundsVerbCount)
{
    vi_msg_t msg = {};
    vii_msg_t call = system_call(V_ORIGIN_EXTERNAL, V_VERB_COUNT, &msg);

    EXPECT_EQ(vii_dispatch(&call), V_STATUS_NOT_IMPLEMENTED);
}

TEST_F(Via, DispatchScaleThirtyTwoAttributes)
{
    v_attr_t attrs[kScale] = {};
    v_status_t statuses[kScale];
    vi_msg_t msg = {};
    vii_msg_t call = system_call(V_ORIGIN_EXTERNAL, V_VERB_GET, &msg);

    for (vuint32_t i = 0; i < kScale; i++) {
        attrs[i].id = VII_SYSTEM_ATTR_NODE_CLASS;
    }
    msg.attr_count = kScale;
    msg.attr_list = attrs;
    msg.status_list = statuses;
    EXPECT_EQ(vii_dispatch(&call), V_STATUS_SUCCESS);
    EXPECT_EQ(msg.attr_count, 1u);
}

constexpr vuint32_t kListMax = 4;
constexpr vuint32_t kTextMax = 8;

enum {
    kListId = 1,
    kTextId = 2,
    kU64Id = 3,
    kTestIdMax = 3,
};

/**
 * @brief Test-only class with an enum list, a bounded text, and a ranged U64.
 *
 * No product class carries a list attribute, so the rows live here.
 */
struct TestClass {
    vint32_t values[3];
    struct vii_attr_meta attrs[3];
    vuint8_t by_id[kTestIdMax + 1];
    struct vii_class cls;
};

/**
 * @brief Fill @p t with the test rows and point its class row at them.
 *
 * @param[out] t Test class storage
 */
void make_test_class(TestClass *t)
{
    std::memset(t, 0, sizeof(*t));
    t->values[0] = 1;
    t->values[1] = 2;
    t->values[2] = 3;

    t->attrs[0].id = kListId;
    t->attrs[0].kind = VII_T_LIST;
    t->attrs[0].elem_kind = VII_T_S32;
    t->attrs[0].elem_size = sizeof(vint32_t);
    t->attrs[0].max_count = kListMax;
    t->attrs[0].value_count = 3;
    t->attrs[0].values = t->values;

    t->attrs[1].id = kTextId;
    t->attrs[1].kind = VII_T_TEXT;
    t->attrs[1].max_len = kTextMax;

    t->attrs[2].id = kU64Id;
    t->attrs[2].kind = VII_T_U64;
    t->attrs[2].has_range = 1;
    t->attrs[2].min = 0;
    t->attrs[2].max = 10;

    t->by_id[kListId] = 1;
    t->by_id[kTextId] = 2;
    t->by_id[kU64Id] = 3;
    t->cls.attr_by_id = t->by_id;
    t->cls.attrs = t->attrs;
    t->cls.attr_id_max = kTestIdMax;
    t->cls.attr_count = 3;
}

/**
 * @brief Check one attribute of the test class from the internal origin.
 *
 * @param[in]     t    Test class
 * @param[in,out] attr Attribute to check
 * @param[in]     verb Verb being checked
 * @return The overall status of vii_attr_check.
 */
v_status_t check_one_test_attr(const TestClass *t, v_attr_t *attr, v_verb_t verb)
{
    v_status_t status = V_STATUS_FAILURE;
    vi_msg_t msg = {};

    msg.attr_count = 1;
    msg.attr_list = attr;
    msg.status_list = &status;
    return vii_attr_check(&t->cls, V_ORIGIN_INTERNAL, &msg, verb);
}

TEST(ViaList, Basic)
{
    TestClass t;
    vint32_t items[kListMax + 2] = {1, 2, 3};
    v_attr_t attr = {};

    make_test_class(&t);
    attr.id = kListId;
    attr.value.list.items = items;
    attr.value.list.count = 3;
    EXPECT_EQ(check_one_test_attr(&t, &attr, V_VERB_SET), V_STATUS_SUCCESS);

    attr.value.list.count = kListMax + 2;
    EXPECT_EQ(check_one_test_attr(&t, &attr, V_VERB_GET), V_STATUS_SUCCESS);
}

TEST(ViaList, NegativeNullItemsWithCount)
{
    TestClass t;
    v_attr_t attr = {};

    make_test_class(&t);
    attr.id = kListId;
    attr.value.list.count = 2;
    EXPECT_EQ(check_one_test_attr(&t, &attr, V_VERB_GET), V_STATUS_INVALID_ATTR_VALUE_0);
    EXPECT_EQ(check_one_test_attr(&t, &attr, V_VERB_SET), V_STATUS_INVALID_ATTR_VALUE_0);
}

TEST(ViaList, BoundsOverMaxCount)
{
    TestClass t;
    vint32_t items[kListMax + 1] = {1, 1, 1, 1, 1};
    v_attr_t attr = {};

    make_test_class(&t);
    attr.id = kListId;
    attr.value.list.items = items;
    attr.value.list.count = kListMax;
    EXPECT_EQ(check_one_test_attr(&t, &attr, V_VERB_SET), V_STATUS_SUCCESS);
    attr.value.list.count = kListMax + 1;
    EXPECT_EQ(check_one_test_attr(&t, &attr, V_VERB_SET), V_STATUS_INVALID_ATTR_VALUE_0);
}

TEST(ViaList, NegativeOneItemOutOfEnum)
{
    TestClass t;
    vint32_t items[3] = {1, 9, 2};
    v_attr_t attr = {};

    make_test_class(&t);
    attr.id = kListId;
    attr.value.list.items = items;
    attr.value.list.count = 3;
    EXPECT_EQ(check_one_test_attr(&t, &attr, V_VERB_SET), V_STATUS_INVALID_ATTR_VALUE_0);
    EXPECT_EQ(check_one_test_attr(&t, &attr, V_VERB_GET), V_STATUS_SUCCESS);
}

TEST(ViaText, BoundsOverMaxLen)
{
    TestClass t;
    char buf[kTextMax + 1] = {};
    v_attr_t attr = {};

    make_test_class(&t);
    attr.id = kTextId;
    attr.value.text.buf = buf;
    attr.value.text.len = kTextMax;
    EXPECT_EQ(check_one_test_attr(&t, &attr, V_VERB_SET), V_STATUS_SUCCESS);
    attr.value.text.len = kTextMax + 1;
    EXPECT_EQ(check_one_test_attr(&t, &attr, V_VERB_SET), V_STATUS_INVALID_ATTR_VALUE_0);
    EXPECT_EQ(check_one_test_attr(&t, &attr, V_VERB_GET), V_STATUS_SUCCESS);
}

TEST(ViaRange, CornerU64AboveSignedMax)
{
    TestClass t;
    v_attr_t attr = {};

    make_test_class(&t);
    attr.id = kU64Id;
    attr.value.u64 = 10;
    EXPECT_EQ(check_one_test_attr(&t, &attr, V_VERB_SET), V_STATUS_SUCCESS);
    attr.value.u64 = UINT64_MAX;
    EXPECT_EQ(check_one_test_attr(&t, &attr, V_VERB_SET), V_STATUS_INVALID_ATTR_VALUE_0);
}

TEST(ViaVerbRules, BasicMatchTheModel)
{
    EXPECT_TRUE(vii_verb_rules[V_VERB_GET].collapse);
    EXPECT_FALSE(vii_verb_rules[V_VERB_GET].writes_values);
    EXPECT_TRUE(vii_verb_rules[V_VERB_SET].writes_values);
    EXPECT_FALSE(vii_verb_rules[V_VERB_DELETE].checks_list);
    EXPECT_TRUE(vii_verb_rules[V_VERB_NOTIFY].writes_values);
    EXPECT_FALSE(vii_verb_rules[V_VERB_NOTIFY].collapse);
}

TEST_F(Via, LookupBounds)
{
    const struct vii_class *cls = system_class();
    const struct vii_attr_meta *m;

    EXPECT_EQ(vii_attr_lookup(cls, 0), nullptr);
    m = vii_attr_lookup(cls, cls->attr_id_max);
    ASSERT_NE(m, nullptr);
    EXPECT_EQ(m->id, cls->attr_id_max);
    EXPECT_EQ(vii_attr_lookup(cls, cls->attr_id_max + 1), nullptr);
    EXPECT_EQ(vii_attr_lookup(&vii_classes[V_CLASS_UNSPECIFIED], 1), nullptr);
}

} // namespace

/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#include "vti/codec.h"

#include <google/protobuf/any.pb.h>
#include <google/protobuf/arena.h>

#include "platform/memory.h"
#include "vedam/system/v1/system.pb.h"
#include "vii/class/system.h"

static_assert(VTI_CODEC_VALUE_MAX == 256,
              "the model checks text and list bounds against this slot size");

static_assert(sizeof(vi_system_state_t) <= VTI_CODEC_VALUE_MAX,
              "vi_system_state_t must fit one codec value slot");

namespace
{

/**
 * @brief Pack the vi_system_state_t at @p c into @p dst as SystemState.
 *
 * The message is built on the arena that owns @p dst.
 *
 * @param[in]  c   C struct
 * @param[out] dst Any inside a workspace message
 * @return V_STATUS_SUCCESS, or V_STATUS_FAILURE when the message cannot be packed.
 */
v_status_t put_vi_system_state_t(const void *c, google::protobuf::Any *dst)
{
    const vi_system_state_t *src = static_cast<const vi_system_state_t *>(c);
    vedam::system::v1::SystemState *msg =
        google::protobuf::Arena::CreateMessage<vedam::system::v1::SystemState>(dst->GetArena());

    msg->set_operational(static_cast<vedam::system::v1::SystemStateKind>(src->operational));
    return dst->PackFrom(*msg) ? V_STATUS_SUCCESS : V_STATUS_FAILURE;
}

/**
 * @brief Unpack SystemState from @p src into the vi_system_state_t at @p c.
 *
 * Members that do not travel are written zero.
 *
 * @param[in]  src Any inside a workspace message
 * @param[out] c   C struct
 * @return V_STATUS_SUCCESS, or V_STATUS_INVALID_PARAMETER when @p src holds another message.
 */
v_status_t take_vi_system_state_t(const google::protobuf::Any &src, void *c)
{
    vi_system_state_t *dst = static_cast<vi_system_state_t *>(c);
    vedam::system::v1::SystemState *msg =
        google::protobuf::Arena::CreateMessage<vedam::system::v1::SystemState>(src.GetArena());

    if (!src.UnpackTo(msg)) {
        return V_STATUS_INVALID_PARAMETER;
    }
    vplatform_memset(dst, 0, sizeof(*dst));
    dst->operational = static_cast<vi_system_state_e>(msg->operational());
    return V_STATUS_SUCCESS;
}

} // namespace

const struct vti_struct_codec vti_struct_codecs[] = {
    {put_vi_system_state_t, take_vi_system_state_t},
};

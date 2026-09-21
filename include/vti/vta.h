/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VTI_VTA_H
#define VTI_VTA_H

#include "vii/types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    VTA_LINK_LOCAL = 0,
    VTA_LINK_IPC = 1,
    VTA_LINK_REMOTE = 2,
    VTA_LINK_COUNT = 3
};

/**
 * @brief Empty descriptor for @p link. The node must already exist.
 *
 * The operations table is the one chosen for that link at vedam_init.
 * The address is applied later by vta_listen or vta_connect.
 */
vta_desc_t *vta_open(int link);

/** @brief Close @p desc. The caller that opened it closes it. */
void vta_close(vta_desc_t *desc);

/** @brief Bind @p desc and wait for callers. */
v_status_t vta_listen(vta_desc_t *desc, const char *address);

/** @brief Accept one caller. The listen thread owns this for byte transports. */
v_status_t vta_accept(vta_desc_t *desc, vta_desc_t **client);

/** @brief Dial @p address. The result is a sender descriptor. */
v_status_t vta_connect(vta_desc_t *desc, const char *address);

/**
 * @brief One verb on @p desc.
 *
 * Runs the verb and writes values and per-attribute status codes into
 * @p msg->msg. Does not close @p desc; the caller that opened it closes it.
 */
v_status_t vta_call(vta_desc_t *desc, vta_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif

/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef V_PLATFORM_MQ_H
#define V_PLATFORM_MQ_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Opaque POSIX message-queue handle.
 */
typedef struct vplatform_mq {
    intptr_t handle;
} vplatform_mq_t;

/**
 * @brief Create or open a receive queue. Does not unlink an existing name.
 * @return 0 on success, -1 on failure.
 */
int vplatform_mq_open_recv(vplatform_mq_t *mq, const char *name, long maxmsg, long msgsize);

/**
 * @brief Open an existing queue for send only.
 * @return 0 on success, -1 on failure.
 */
int vplatform_mq_open_send(vplatform_mq_t *mq, const char *name);

/**
 * @brief Create a fresh receive queue and write its name into @p name.
 * @return 0 on success, -1 on failure.
 */
int vplatform_mq_open_reply(vplatform_mq_t *mq, char *name, size_t name_cap);

/**
 * @brief Receive one message into @p buf. Returns byte count, or -1 on error.
 */
int vplatform_mq_receive(vplatform_mq_t *mq, char *buf, size_t buf_size);

/**
 * @brief Send one message. Returns 0 on success, -1 on failure.
 */
int vplatform_mq_send(vplatform_mq_t *mq, const void *buf, size_t len);

/**
 * @brief Close a send queue. Does not unlink the name.
 */
void vplatform_mq_close_send(vplatform_mq_t *mq);

/**
 * @brief Close and unlink the queue named at open.
 */
void vplatform_mq_close(vplatform_mq_t *mq, const char *name);

#endif

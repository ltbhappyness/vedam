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
 *
 * @param[out] mq      Receives the queue handle
 * @param[in]  name    Queue name
 * @param[in]  maxmsg  Maximum number of messages
 * @param[in]  msgsize Maximum message size
 * @return 0 on success, -1 on failure.
 */
int vplatform_mq_open_recv(vplatform_mq_t *mq, const char *name, long maxmsg, long msgsize);

/**
 * @brief Open an existing queue for send only.
 *
 * @param[out] mq   Receives the queue handle
 * @param[in]  name Queue name
 * @return 0 on success, -1 on failure.
 */
int vplatform_mq_open_send(vplatform_mq_t *mq, const char *name);

/**
 * @brief Create a fresh receive queue and write its name into @p name.
 *
 * @param[out] mq       Receives the queue handle
 * @param[out] name     Receives the queue name
 * @param[in]  name_cap Size of @p name
 * @return 0 on success, -1 on failure.
 */
int vplatform_mq_open_reply(vplatform_mq_t *mq, char *name, size_t name_cap);

/**
 * @brief Receive one message into @p buf.
 *
 * @param[in]  mq       Queue handle
 * @param[out] buf      Buffer for the message
 * @param[in]  buf_size Size of @p buf
 * @return Byte count, or -1 on error.
 */
int vplatform_mq_receive(vplatform_mq_t *mq, char *buf, size_t buf_size);

/**
 * @brief Send one message.
 *
 * @param[in] mq  Queue handle
 * @param[in] buf Message bytes
 * @param[in] len Number of bytes
 * @return 0 on success, -1 on failure.
 */
int vplatform_mq_send(vplatform_mq_t *mq, const void *buf, size_t len);

/**
 * @brief Close a send queue. Does not unlink the name.
 *
 * @param[in,out] mq Queue handle. The descriptor is closed.
 */
void vplatform_mq_close_send(vplatform_mq_t *mq);

/**
 * @brief Close and unlink the queue named at open.
 *
 * @param[in,out] mq   Queue handle. The descriptor is closed.
 * @param[in]     name Queue name to unlink
 */
void vplatform_mq_close(vplatform_mq_t *mq, const char *name);

#endif

/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "platform/mq.h"

#if VEDAM_ENABLE_MQ

#if defined(__linux__) || defined(VEDAM_OS_LINUX)

#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int vplatform_mq_open_recv(vplatform_mq_t *mq, const char *name, long maxmsg, long msgsize)
{
    struct mq_attr attr;
    mqd_t mqd;

    if (mq == NULL || name == NULL || name[0] == '\0') {
        return -1;
    }
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = maxmsg;
    attr.mq_msgsize = msgsize;
    mqd = mq_open(name, O_CREAT | O_RDONLY | O_NONBLOCK, 0600, &attr);
    if (mqd == (mqd_t)-1) {
        return -1;
    }
    mq->handle = (intptr_t)mqd;
    return 0;
}

/**
 * @brief Open a POSIX message queue for sending.
 * @param[in] mq Argument mq
 * @param[in] name Argument name
 * @return Result of vplatform_mq_open_send.
 */
int vplatform_mq_open_send(vplatform_mq_t *mq, const char *name)
{
    mqd_t mqd;

    if (mq == NULL || name == NULL || name[0] == '\0') {
        return -1;
    }
    mqd = mq_open(name, O_WRONLY);
    if (mqd == (mqd_t)-1) {
        return -1;
    }
    mq->handle = (intptr_t)mqd;
    return 0;
}

/**
 * @brief Create a private reply queue with an exclusive name.
 * @param[in] mq Argument mq
 * @param[in] name Argument name
 * @param[in] name_cap Argument name_cap
 * @return Result of vplatform_mq_open_reply.
 */
int vplatform_mq_open_reply(vplatform_mq_t *mq, char *name, size_t name_cap)
{
    struct mq_attr attr;
    mqd_t mqd;
    unsigned n;
    int nprint;

    if (mq == NULL || name == NULL || name_cap < 16) {
        return -1;
    }
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 8192;
    for (n = 0; n < 1000u; n++) {
        nprint = snprintf(name, name_cap, "/vr%x%x", (unsigned)getpid(), n);
        if (nprint < 0 || (size_t)nprint >= name_cap) {
            return -1;
        }
        mqd = mq_open(name, O_CREAT | O_EXCL | O_RDONLY, 0600, &attr);
        if (mqd != (mqd_t)-1) {
            mq->handle = (intptr_t)mqd;
            return 0;
        }
    }
    return -1;
}

/**
 * @brief Receive one message from a queue.
 * @param[in] mq Argument mq
 * @param[in] buf Argument buf
 * @param[in] buf_size Argument buf_size
 * @return Result of vplatform_mq_receive.
 */
int vplatform_mq_receive(vplatform_mq_t *mq, char *buf, size_t buf_size)
{
    mqd_t mqd;
    ssize_t n;

    if (mq == NULL || buf == NULL || buf_size == 0) {
        return -1;
    }
    mqd = (mqd_t)mq->handle;
    n = mq_receive(mqd, buf, buf_size, NULL);
    return (int)n;
}

/**
 * @brief Send one message on a queue.
 * @param[in] mq Argument mq
 * @param[in] buf Argument buf
 * @param[in] len Argument len
 * @return Result of vplatform_mq_send.
 */
int vplatform_mq_send(vplatform_mq_t *mq, const void *buf, size_t len)
{
    mqd_t mqd;
    int rc;

    if (mq == NULL || buf == NULL || len == 0) {
        return -1;
    }
    mqd = (mqd_t)mq->handle;
    rc = mq_send(mqd, (const char *)buf, len, 0);
    if (rc != 0) {
        return -1;
    }
    return 0;
}

/**
 * @brief Close a send-only queue handle.
 * @param[in] mq Argument mq
 */
void vplatform_mq_close_send(vplatform_mq_t *mq)
{
    mqd_t mqd;

    if (mq == NULL) {
        return;
    }
    mqd = (mqd_t)mq->handle;
    if (mqd != (mqd_t)-1) {
        mq_close(mqd);
        mq->handle = (intptr_t)(mqd_t)-1;
    }
}

/**
 * @brief Close a queue handle and unlink its name when this process created it.
 * @param[in] mq Argument mq
 * @param[in] name Argument name
 */
void vplatform_mq_close(vplatform_mq_t *mq, const char *name)
{
    mqd_t mqd;

    if (mq == NULL) {
        return;
    }
    mqd = (mqd_t)mq->handle;
    if (mqd != (mqd_t)-1) {
        mq_close(mqd);
        mq->handle = (intptr_t)(mqd_t)-1;
    }
    if (name != NULL && name[0] != '\0') {
        mq_unlink(name);
    }
}

#else
#error "vplatform mq: unsupported OS"
#endif

#endif /* VEDAM_ENABLE_MQ */

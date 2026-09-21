/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "platform/thread.h"

#if defined(__linux__) || defined(VEDAM_OS_LINUX)

#include <pthread.h>
#include <stdlib.h>

int vplatform_thread_create(vplatform_thread_t *thread, void *(*entry)(void *), void *arg)
{
    pthread_t *th;
    int rc;

    if (thread == NULL || entry == NULL) {
        return -1;
    }

    th = (pthread_t *)malloc(sizeof(*th));
    if (th == NULL) {
        return -1;
    }
    rc = pthread_create(th, NULL, entry, arg);
    if (rc != 0) {
        free(th);
        return -1;
    }
    thread->handle = th;
    return 0;
}

/**
 * @brief Join a platform thread.
 * @param[in] thread Argument thread
 * @return Result of vplatform_thread_join.
 */
int vplatform_thread_join(vplatform_thread_t *thread)
{
    pthread_t *th;
    int rc;

    if (thread == NULL || thread->handle == NULL) {
        return -1;
    }

    th = (pthread_t *)thread->handle;
    rc = pthread_join(*th, NULL);
    if (rc != 0) {
        return -1;
    }
    free(th);
    thread->handle = NULL;
    return 0;
}

/**
 * @brief Detach a platform thread.
 * @param[in] thread Argument thread
 * @return Result of vplatform_thread_detach.
 */
int vplatform_thread_detach(vplatform_thread_t *thread)
{
    pthread_t *th;
    int rc;

    if (thread == NULL || thread->handle == NULL) {
        return -1;
    }

    th = (pthread_t *)thread->handle;
    rc = pthread_detach(*th);
    if (rc != 0) {
        return -1;
    }
    free(th);
    thread->handle = NULL;
    return 0;
}

#else
#error "vplatform thread: unsupported OS"
#endif

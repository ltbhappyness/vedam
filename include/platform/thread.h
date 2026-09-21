/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef V_PLATFORM_THREAD_H
#define V_PLATFORM_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque thread handle for this process.
 */
typedef struct vplatform_thread {
    void *handle;
} vplatform_thread_t;

/**
 * @brief Start a detached-from-caller joinable thread.
 * @return 0 on success, non-zero on failure.
 */
int vplatform_thread_create(vplatform_thread_t *thread, void *(*entry)(void *), void *arg);

/**
 * @brief Wait for @p thread to finish.
 * @return 0 on success, non-zero on failure.
 */
int vplatform_thread_join(vplatform_thread_t *thread);

/**
 * @brief Detach @p thread so it releases its own resources on exit.
 * @return 0 on success, non-zero on failure.
 */
int vplatform_thread_detach(vplatform_thread_t *thread);

#ifdef __cplusplus
}
#endif

#endif

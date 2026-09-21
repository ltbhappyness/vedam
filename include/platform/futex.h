/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef V_PLATFORM_FUTEX_H
#define V_PLATFORM_FUTEX_H

#include <stdint.h>

/**
 * @brief Sleep while @p word still equals @p expect.
 *
 * A wake that changes @p word first is not lost. A spurious return is allowed.
 * @return 0.
 */
int vplatform_futex_wait(uint32_t *word, uint32_t expect);

/**
 * @brief Wake up to @p n sleepers blocked in vplatform_futex_wait on @p word.
 *
 * @return 0.
 */
int vplatform_futex_wake(uint32_t *word, int n);

#endif

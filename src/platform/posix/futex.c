/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "platform/futex.h"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

int vplatform_futex_wait(uint32_t *word, uint32_t expect)
{
    syscall(SYS_futex, word, (int)(FUTEX_WAIT | FUTEX_PRIVATE_FLAG), expect, NULL, NULL, 0);
    return 0;
}

/**
 * @brief Wake waiters on a futex word.
 * @param[in] word Argument word
 * @param[in] n Argument n
 * @return Result of vplatform_futex_wake.
 */
int vplatform_futex_wake(uint32_t *word, int n)
{
    syscall(SYS_futex, word, (int)(FUTEX_WAKE | FUTEX_PRIVATE_FLAG), n, NULL, NULL, 0);
    return 0;
}

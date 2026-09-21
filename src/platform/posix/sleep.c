/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "platform/sleep.h"

#if defined(__linux__) || defined(VEDAM_OS_LINUX)

#include <poll.h>
#include <stddef.h>

void vplatform_sleep_idle(int timeout_ms)
{
    (void)poll(NULL, 0, timeout_ms < 0 ? 0 : timeout_ms);
}

#else
#error "vplatform sleep: unsupported OS"
#endif

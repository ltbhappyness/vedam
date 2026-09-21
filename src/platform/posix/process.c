/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "platform/process.h"

#if defined(__linux__) || defined(VEDAM_OS_LINUX)

#include <unistd.h>

int vplatform_daemonize(int foreground)
{
    int rc;

    if (foreground) {
        return 0;
    }
    rc = daemon(0, 0);
    return rc;
}

/**
 * @brief Return the process id.
 * @return Result of vplatform_pid.
 */
long vplatform_pid(void)
{
    return (long)getpid();
}

#else
#error "vplatform process: unsupported OS"
#endif

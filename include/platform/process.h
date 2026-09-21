/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef V_PLATFORM_PROCESS_H
#define V_PLATFORM_PROCESS_H

/**
 * @brief Detach into the background when @p foreground is zero.
 * @return 0 on success, -1 on failure.
 */
int vplatform_daemonize(int foreground);

/**
 * @brief Process id of this process.
 */
long vplatform_pid(void);

#endif

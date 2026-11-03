/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef V_PLATFORM_SLEEP_H
#define V_PLATFORM_SLEEP_H

/**
 * @brief Block without spinning until @p timeout_ms elapses or the wait is interrupted.
 *
 * @param[in] timeout_ms Milliseconds to block
 */
void vplatform_sleep_idle(int timeout_ms);

#endif

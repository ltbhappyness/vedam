/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef V_UTIL_TRACE_H
#define V_UTIL_TRACE_H

#include <stdio.h>

#define vtrace_enter() fprintf(stdout, "trace %s:%d enter\n", __func__, __LINE__)
#define vtrace_exit() fprintf(stdout, "trace %s:%d exit\n", __func__, __LINE__)
#define vtrace_state(fmt, ...) \
    fprintf(stdout, "trace %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)

#endif

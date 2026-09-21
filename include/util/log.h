/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef V_UTIL_LOG_H
#define V_UTIL_LOG_H

#include <stdio.h>

#define vlog_error(fmt, ...) \
    fprintf(stdout, "error %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define vlog_warn(fmt, ...) \
    fprintf(stdout, "warn %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define vlog_info(fmt, ...) \
    fprintf(stdout, "info %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define vlog_debug(fmt, ...) \
    fprintf(stdout, "debug %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)

#endif

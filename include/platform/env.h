/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef V_PLATFORM_ENV_H
#define V_PLATFORM_ENV_H

/**
 * @brief Read an environment variable. Returns NULL when unset or empty.
 */
const char *vplatform_getenv(const char *name);

#endif

/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "platform/env.h"

#if defined(__linux__) || defined(VEDAM_OS_LINUX)

#include <stdlib.h>

const char *vplatform_getenv(const char *name)
{
    const char *value;

    if (name == NULL) {
        return NULL;
    }
    value = getenv(name);
    if (value == NULL || value[0] == '\0') {
        return NULL;
    }
    return value;
}

#else
#error "vplatform env: unsupported OS"
#endif

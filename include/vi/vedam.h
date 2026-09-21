/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * THIS IS GENERATED CODE AND DO NOT EDIT
 */
#ifndef VI_VEDAM_H
#define VI_VEDAM_H

#include "vi/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Allocate the node, start transport receive paths, then run create.
 */
v_status_t vedam_init(void);

/**
 * @brief Tear down transports and delete System.
 */
v_status_t vedam_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* VI_VEDAM_H */

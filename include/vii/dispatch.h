/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VII_DISPATCH_H
#define VII_DISPATCH_H

#include "vii/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enter the node for @p call under the Wait-Die lock map.
 *
 * Local vi_*, init, and a transport receive that already holds a vi_msg_t
 * call this. vedam_lock_invoke takes the locks, then the class verb runs.
 * A library that needs another vertical calls this again with internal
 * origin. That call joins the attempt already running on the thread. A
 * Wait-Die die returns VEDAM_LOCK_RESTART and the outermost attempt starts
 * again. A send uses vedam_node_call after that attempt releases its locks.
 *
 * @param[in,out] call Internal call (class, verb, origin, version, and vi_msg_t)
 * @return The verb status, or V_STATUS_NOT_IMPLEMENTED when the class or verb is unknown.
 */
v_status_t vii_dispatch(vii_msg_t *call);

#ifdef __cplusplus
}
#endif

#endif /* VII_DISPATCH_H */

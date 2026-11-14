/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef VTI_MQ_H
#define VTI_MQ_H

#include "vti/vti.h"

extern const vti_ops_t vti_mq_ops;

/**
 * @brief Queue name the listening descriptor bound, or NULL.
 */
const char *vti_mq_bound(vti_desc_t *desc);

#endif

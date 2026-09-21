/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#ifndef V_PLATFORM_H
#define V_PLATFORM_H

/**
 * @file platform.h
 * @brief Umbrella for OS helpers. Call sites use vplatform_*; libc stays under src/platform/.
 *
 * Categories (headers under include/platform/, POSIX bodies under src/platform/posix/):
 * - memory  — calloc, free, memset, memcpy
 * - string  — bounded copy
 * - thread  — create / join
 * - sleep   — idle wait
 * - futex   — sleep and wake on one word
 * - mq      — POSIX message queue open / receive / close
 * - env     — getenv
 * - process — daemonize
 *
 * Socket / TCP / UDS for the temporary hand HTTP path still live in vta/grpc until
 * official gRPC / Connect replace that stack.
 */

#include "platform/memory.h"
#include "platform/string.h"
#include "platform/thread.h"
#include "platform/sleep.h"
#include "platform/futex.h"
#include "platform/mq.h"
#include "platform/env.h"
#include "platform/process.h"

#endif

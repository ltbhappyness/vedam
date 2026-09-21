/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * Compile-time node profile (defconfig.class-*).
 *
 * Plan (later CMake step, not wired yet):
 * 1. cmake -B build -DVEDAM_CONFIG_FILE=config/defconfig.class-c
 * 2. A small parser turns KEY=value lines into a generated
 *    build/vedam_config.h (VEDAM_NODE_CLASS_C, VEDAM_ENABLE_MQ, …).
 * 3. Those macros gate optional sources (MQ, gRPC, Connect) and default
 *    transport choices when the matching environment variable is unset.
 * 4. Runtime VEDAM_TRANSPORT_* still overrides for tests.
 * 5. Class letters A–E each keep their own defconfig file.
 *
 * Today the files are the source of truth for intent only. CMake always
 * builds every transport stub.
 */

/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "vi/vedam.h"

#include "platform/process.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t g_stop;

/**
 * @brief Record a stop request from a signal.
 * @param[in] sig Argument sig
 */
static void on_signal(int sig)
{
    (void)sig;
    g_stop = 1;
}

/**
 * @brief Run the vedamd process.
 * @param[in] argc Argument argc
 * @param[in] argv Argument argv
 * @return Result of main.
 */
int main(int argc, char **argv)
{
    int foreground = 0;
    int i;
    int started;
    v_status_t rc;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            foreground = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            fprintf(stderr, "usage: vedamd [-f]\n");
            return 0;
        }
    }

    started = vplatform_daemonize(foreground);
    if (started != 0) {
        perror("daemon");
        return 1;
    }

    signal(SIGTERM, on_signal);
    signal(SIGINT, on_signal);

    rc = vedam_init();
    if (rc != V_STATUS_SUCCESS) {
        fprintf(stderr, "vedam_init failed: %d\n", (int)rc);
        return 1;
    }

    while (!g_stop) {
        pause();
    }

    vedam_deinit();
    return 0;
}

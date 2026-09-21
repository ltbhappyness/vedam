/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "vi/class/system.h"
#include "vi/vedam.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    v_attr_t a;
    v_status_t status;
    vi_msg_t msg;
    v_status_t st;

    st = vedam_init();
    if (st != V_STATUS_SUCCESS) {
        fprintf(stderr, "vedam_init %d\n", (int)st);
        return 1;
    }

    memset(&a, 0, sizeof(a));
    memset(&status, 0, sizeof(status));
    memset(&msg, 0, sizeof(msg));
    a.id = VI_SYSTEM_ATTR_PROTOCOL_VERSION;
    msg.attr_count = 1;
    msg.attr_list = &a;
    msg.status_list = &status;
    st = vi_system_get(&msg);
    if (st != V_STATUS_SUCCESS) {
        fprintf(stderr, "vi_system_get %d\n", (int)st);
        vedam_deinit();
        return 1;
    }

    printf("protocol %u.%u\n", a.value.version.major, a.value.version.minor);
    vedam_deinit();
    return 0;
}

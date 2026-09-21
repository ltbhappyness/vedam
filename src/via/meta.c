/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "meta.h"

#include <stddef.h>

const struct via_attr_meta *via_attr_find(const struct via_attr_meta *tab, vuint32_t n,
                                          v_attr_id_t id)
{
    vuint32_t i;

    if (tab == NULL) {
        return NULL;
    }
    for (i = 0; i < n; i++) {
        if (tab[i].id == id) {
            return &tab[i];
        }
    }
    return NULL;
}

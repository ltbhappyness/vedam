/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "ds/list.h"

/**
 * @brief Count nodes on @p head. The head itself is not counted.
 *
 * @param[in] head List head
 * @return Number of nodes.
 */
vuint32_t vds_list_count(const struct vds_list *head)
{
    const struct vds_list *pos;
    vuint32_t count = 0;

    if (head == NULL) {
        return 0;
    }
    vds_list_for_each (pos, head) {
        count++;
    }
    return count;
}

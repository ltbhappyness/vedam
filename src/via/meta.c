/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "vii/meta.h"

/**
 * @brief Return non-zero when the integer at @p p lies in the range of @p m.
 *
 * @param[in] m    Attribute row with has_range set
 * @param[in] kind VII_T_* kind of the value at @p p
 * @param[in] p    One value or one list item
 * @return Non-zero when the value is in range.
 */
static int in_range(const struct vii_attr_meta *m, vuint8_t kind, const void *p)
{
    vint64_t v;

    switch (kind) {
    case VII_T_S32:
        v = *(const vint32_t *)p;
        break;
    case VII_T_U32:
        v = *(const vuint32_t *)p;
        break;
    case VII_T_U64:
        if (*(const vuint64_t *)p > (vuint64_t)INT64_MAX) {
            return 0;
        }
        v = (vint64_t)(*(const vuint64_t *)p);
        break;
    default:
        return 1;
    }
    return v >= m->min && v <= m->max;
}

/**
 * @brief Return non-zero when the enum value at @p p is one of the values of @p m.
 *
 * @param[in] m Attribute row with values set
 * @param[in] p One value or one list item
 * @return Non-zero when the value is a member.
 */
static int is_member(const struct vii_attr_meta *m, const void *p)
{
    vint32_t v = *(const vint32_t *)p;
    vuint32_t i;

    for (i = 0; i < m->value_count; i++) {
        if (m->values[i] == v) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Return non-zero when one value or one list item meets the range and enum of @p m.
 *
 * @param[in] m    Attribute row
 * @param[in] kind VII_T_* kind of the value at @p p
 * @param[in] p    One value or one list item
 * @return Non-zero when the value is allowed.
 */
static int item_ok(const struct vii_attr_meta *m, vuint8_t kind, const void *p)
{
    if (m->has_range && !in_range(m, kind, p)) {
        return 0;
    }
    if (m->values != NULL && !is_member(m, p)) {
        return 0;
    }
    return 1;
}

/**
 * @brief Return non-zero when the buffer fields of @p attr are usable for @p verb.
 *
 * A text or list with a length or count needs its buffer. A struct needs its
 * pointer on get.
 *
 * @param[in] m    Attribute row
 * @param[in] attr Attribute value
 * @param[in] verb Verb being checked
 * @return Non-zero when the shape is usable.
 */
static int shape_ok(const struct vii_attr_meta *m, const v_attr_t *attr, v_verb_t verb)
{
    switch (m->kind) {
    case VII_T_TEXT:
        return attr->value.text.len == 0 || attr->value.text.buf != NULL;
    case VII_T_LIST:
        return attr->value.list.count == 0 || attr->value.list.items != NULL;
    case VII_T_STRUCT:
        return verb != V_VERB_GET || attr->value.ptr != NULL;
    default:
        return 1;
    }
}

/**
 * @brief Return non-zero when a written value meets the limits of @p m.
 *
 * Text is held to max_len. A list is held to max_count, and every item to the
 * range and enum of the row. Any other kind is one item.
 *
 * @param[in] m    Attribute row
 * @param[in] attr Attribute value
 * @return Non-zero when the value is allowed.
 */
static int limits_ok(const struct vii_attr_meta *m, const v_attr_t *attr)
{
    const vuint8_t *item;
    vuint32_t i;

    switch (m->kind) {
    case VII_T_TEXT:
        return m->max_len == 0 || attr->value.text.len <= m->max_len;
    case VII_T_LIST:
        if (attr->value.list.count > m->max_count) {
            return 0;
        }
        item = attr->value.list.items;
        for (i = 0; i < attr->value.list.count; i++) {
            if (!item_ok(m, m->elem_kind, item + (size_t)i * m->elem_size)) {
                return 0;
            }
        }
        return 1;
    default:
        return item_ok(m, m->kind, &attr->value);
    }
}

/**
 * @brief Check one attribute value against its row for @p verb.
 *
 * The shape is checked on every verb. On get, a text len or a list count is
 * the room the caller gives. The limits are checked only when the verb carries
 * values from the caller.
 *
 * @param[in] m     Attribute row
 * @param[in] attr  Attribute value
 * @param[in] verb  Verb being checked
 * @param[in] index Attribute index in the caller's list
 * @return V_STATUS_SUCCESS or an indexed invalid-value status.
 */
static v_status_t check_value(const struct vii_attr_meta *m, const v_attr_t *attr, v_verb_t verb,
                              vint32_t index)
{
    v_status_t bad = (v_status_t)(V_STATUS_INVALID_ATTR_VALUE_0 - index);

    if (!shape_ok(m, attr, verb)) {
        return bad;
    }
    if (!vii_verb_rules[verb].writes_values) {
        return V_STATUS_SUCCESS;
    }
    if (!limits_ok(m, attr)) {
        return bad;
    }
    return V_STATUS_SUCCESS;
}

/**
 * @brief Check one attribute id against its row, the origin, and the verb.
 *
 * @param[in] m      Attribute row, or NULL when the class does not define the id
 * @param[in] origin Call origin
 * @param[in] attr   Attribute value
 * @param[in] verb   Verb being checked
 * @param[in] index  Attribute index in the caller's list
 * @return V_STATUS_SUCCESS or an indexed failure status.
 */
static v_status_t check_one(const struct vii_attr_meta *m, v_origin_t origin, const v_attr_t *attr,
                            v_verb_t verb, vint32_t index)
{
    int external = origin == V_ORIGIN_EXTERNAL;
    int subscribe = verb == V_VERB_SUBSCRIBE || verb == V_VERB_UNSUBSCRIBE || verb == V_VERB_NOTIFY;

    if (m == NULL) {
        return (v_status_t)(V_STATUS_UNKNOWN_ATTRIBUTE_0 - index);
    }
    if (external && !m->is_public) {
        return (v_status_t)(V_STATUS_ATTR_NOT_SUPPORTED_0 - index);
    }
    if (external && (verb == V_VERB_SET || verb == V_VERB_CREATE) && (m->flags & VII_F_READ_ONLY)) {
        return (v_status_t)(V_STATUS_ATTR_NOT_SUPPORTED_0 - index);
    }
    if (verb == V_VERB_SET && (m->flags & VII_F_CREATE_ONLY)) {
        return (v_status_t)(V_STATUS_ATTR_NOT_SUPPORTED_0 - index);
    }
    if (verb == V_VERB_CREATE && (m->flags & VII_F_SET_ONLY)) {
        return (v_status_t)(V_STATUS_ATTR_NOT_SUPPORTED_0 - index);
    }
    if (subscribe && !(m->flags & VII_F_SUBSCRIBABLE)) {
        return (v_status_t)(V_STATUS_ATTR_NOT_SUPPORTED_0 - index);
    }
    return check_value(m, attr, verb, index);
}

/**
 * @brief Return V_STATUS_MANDATORY_ATTR_MISSING when a mandatory-on-create id is absent.
 *
 * @param[in] cls Class row
 * @param[in] msg Call message
 * @return V_STATUS_SUCCESS or V_STATUS_MANDATORY_ATTR_MISSING.
 */
static v_status_t check_mandatory(const struct vii_class *cls, const vi_msg_t *msg)
{
    vuint32_t i;
    vuint32_t j;

    for (j = 0; j < cls->attr_count; j++) {
        if (!(cls->attrs[j].flags & VII_F_MANDATORY_ON_CREATE)) {
            continue;
        }
        for (i = 0; i < msg->attr_count; i++) {
            if (msg->attr_list[i].id == cls->attrs[j].id) {
                break;
            }
        }
        if (i == msg->attr_count) {
            return V_STATUS_MANDATORY_ATTR_MISSING;
        }
    }
    return V_STATUS_SUCCESS;
}

/**
 * @brief Visit every attribute, write status_list[i], and return the first failure.
 *
 * Does not return on the first failure. When every entry is success, checks
 * mandatory-on-create attributes for create.
 *
 * @param[in]     cls    Class row
 * @param[in]     origin Call origin
 * @param[in,out] msg    Call message with attr_list and status_list
 * @param[in]     verb   Verb being checked
 * @return V_STATUS_SUCCESS, the first per-attribute failure, or
 *         V_STATUS_MANDATORY_ATTR_MISSING.
 */
v_status_t vii_attr_check(const struct vii_class *cls, v_origin_t origin, vi_msg_t *msg,
                          v_verb_t verb)
{
    const struct vii_attr_meta *m;
    v_status_t first;
    v_status_t st;
    vuint32_t i;

    first = V_STATUS_SUCCESS;
    for (i = 0; i < msg->attr_count; i++) {
        m = vii_attr_lookup(cls, msg->attr_list[i].id);
        st = check_one(m, origin, &msg->attr_list[i], verb, (vint32_t)i);
        msg->status_list[i] = st;
        if (st != V_STATUS_SUCCESS && first == V_STATUS_SUCCESS) {
            first = st;
        }
    }
    if (first != V_STATUS_SUCCESS) {
        return first;
    }
    if (verb == V_VERB_CREATE) {
        return check_mandatory(cls, msg);
    }
    return V_STATUS_SUCCESS;
}

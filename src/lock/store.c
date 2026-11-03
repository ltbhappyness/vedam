/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
#include "lock/store.h"

#include "core/core.h"
#include "ds/hash.h"
#include "lock/lock.h"
#include "platform/platform.h"
#include "vti/class_uuid.h"

#include <stdatomic.h>

struct vedam_lock_object {
    struct vedam_lock_object *next;
    _Atomic uint32_t gate;
    _Atomic unsigned ref;
    int dead;
    int is_singleton;
    v_class_id_t class_id;
    unsigned char *hash_key;
    vuint32_t hash_len;
    vedam_lock_t *existence;
    vedam_lock_t **attrs;
    vuint32_t attr_cap;
};

struct vedam_lockdb {
    vedam_lock_group_t *group;
    struct vedam_lock_object *singleton[V_CLASS_ID_MAX + 1];
    vds_hash_t *keyed[V_CLASS_ID_MAX + 1];
    struct vedam_lock_object *all;
    _Atomic uint32_t gate;
    _Atomic unsigned long miss_count;
};

struct pin {
    struct vedam_lock_object *obj;
    struct pin *next;
};

struct invoke_args {
    struct vedam_lockdb *db;
    v_class_id_t class_id;
    const void *key;
    vuint32_t key_len;
    v_verb_t verb;
    vuint32_t attr_count;
    v_attr_t *attr_list;
    v_status_t *status_list;
    vedam_lock_body_fn body;
    void *user;
    struct vedam_lock_object *obj;
    int early;
    v_status_t early_status;
};

static _Thread_local int tls_depth;
static _Thread_local struct pin *tls_pins;

enum {
    VEDAM_LOCK_KEY_MAX = 4096,
    VEDAM_LOCK_ATTR_MAX = 64,
    VEDAM_LOCK_LIST_MAX = 32,
    VEDAM_LOCK_ID_STACK = 32
};

int vedam_lock_attr_known(v_class_id_t class_id, v_attr_id_t id);

/**
 * @brief Return non-zero while this thread is inside vedam_lock_invoke.
 * @return Result of vedam_lock_in_attempt.
 */
int vedam_lock_in_attempt(void)
{
    return tls_depth > 0;
}

/**
 * @brief Return the number of lock objects allocated on a miss.
 * @return Result of vedam_lock_object_misses.
 */
unsigned long vedam_lock_object_misses(void)
{
    vedam_t *node = vedam_node();

    if (node == NULL || node->locks == NULL) {
        return 0;
    }
    return atomic_load_explicit(&node->locks->miss_count, memory_order_relaxed);
}

/**
 * @brief Hint the CPU that this spin loop is idle.
 */
static void cpu_relax(void)
{
#if defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__("pause");
#elif defined(__aarch64__)
    __asm__ __volatile__("yield");
#else
    atomic_signal_fence(memory_order_seq_cst);
#endif
}

/**
 * @brief Acquire a spin gate.
 * @param[in] gate Argument gate
 */
static void spin_lock(_Atomic uint32_t *gate)
{
    for (;;) {
        uint32_t zero = 0;

        if (atomic_compare_exchange_weak_explicit(gate, &zero, 1, memory_order_acquire,
                                                  memory_order_relaxed)) {
            return;
        }
        while (atomic_load_explicit(gate, memory_order_relaxed) != 0) {
            cpu_relax();
        }
    }
}

/**
 * @brief Release a spin gate.
 * @param[in] gate Argument gate
 */
static void spin_unlock(_Atomic uint32_t *gate)
{
    atomic_store_explicit(gate, 0, memory_order_release);
}

/**
 * @brief Return non-zero when @p verb takes attribute locks.
 * @param[in] verb Argument verb
 * @return Result of locks_verb.
 */
static int locks_verb(v_verb_t verb)
{
    return verb == V_VERB_CREATE || verb == V_VERB_GET || verb == V_VERB_SET ||
           verb == V_VERB_DELETE || verb == V_VERB_SUBSCRIBE || verb == V_VERB_UNSUBSCRIBE ||
           verb == V_VERB_NOTIFY;
}

/**
 * @brief Return non-zero when @p verb needs an exclusive existence lock.
 * @param[in] verb Argument verb
 * @return Result of exist_exclusive.
 */
static int exist_exclusive(v_verb_t verb)
{
    return verb == V_VERB_CREATE || verb == V_VERB_DELETE;
}

/**
 * @brief Return non-zero when @p verb needs exclusive attribute locks.
 * @param[in] verb Argument verb
 * @return Result of attr_exclusive.
 */
static int attr_exclusive(v_verb_t verb)
{
    return verb == V_VERB_SET || verb == V_VERB_DELETE;
}

/**
 * @brief Copy an object key into a heap buffer.
 * @param[in] key Argument key
 * @param[in] key_len Argument key_len
 * @param[in] out Argument out
 * @param[in] out_len Argument out_len
 * @param[in] stack Argument stack
 * @param[in] stack_cap Argument stack_cap
 * @return Result of copy_object_key.
 */
static int copy_object_key(const void *key, vuint32_t key_len, unsigned char **out,
                           vuint32_t *out_len, unsigned char *stack, vuint32_t stack_cap)
{
    unsigned char *buf;

    if (key == NULL && key_len != 0) {
        return -1;
    }
    if (key_len > VEDAM_LOCK_KEY_MAX) {
        return -1;
    }
    if (key_len == 0) {
        *out = stack;
        *out_len = 0;
        return 0;
    }
    if (key_len <= stack_cap) {
        buf = stack;
    } else {
        buf = vplatform_calloc(1, key_len);
        if (buf == NULL) {
            return -2;
        }
    }
    vplatform_memcpy(buf, key, key_len);
    *out = buf;
    *out_len = key_len;
    return 0;
}

/**
 * @brief Free one lock-map object and its locks.
 * @param[in] obj Argument obj
 */
static void object_destroy(struct vedam_lock_object *obj)
{
    vuint32_t i;

    if (obj == NULL) {
        return;
    }
    vplatform_free(obj->hash_key);
    (void)vedam_lock_destroy(obj->existence);
    for (i = 0; i < obj->attr_cap; i++) {
        (void)vedam_lock_destroy(obj->attrs[i]);
    }
    vplatform_free(obj->attrs);
    vplatform_free(obj);
}

/**
 * @brief Find a pinned object in the current attempt.
 * @param[in] obj Argument obj
 * @return Result of find_pin.
 */
static struct pin *find_pin(struct vedam_lock_object *obj)
{
    struct pin *pin;

    for (pin = tls_pins; pin != NULL; pin = pin->next) {
        if (pin->obj == obj) {
            return pin;
        }
    }
    return NULL;
}

/**
 * @brief Drop one pin and free a dead object when the last pin leaves.
 * @param[in] db Argument db
 * @param[in] obj Argument obj
 */
static void unpin_obj(struct vedam_lockdb *db, struct vedam_lock_object *obj)
{
    struct pin **link = &tls_pins;
    unsigned prev;
    int drop = 0;

    while (*link != NULL) {
        if ((*link)->obj == obj) {
            struct pin *pin = *link;

            *link = pin->next;
            vplatform_free(pin);
            break;
        }
        link = &(*link)->next;
    }
    spin_lock(&db->gate);
    prev = atomic_fetch_sub_explicit(&obj->ref, 1, memory_order_acq_rel);
    if (prev == 1 && obj->dead) {
        struct vedam_lock_object **all = &db->all;

        while (*all != NULL) {
            if (*all == obj) {
                *all = obj->next;
                break;
            }
            all = &(*all)->next;
        }
        if (obj->is_singleton) {
            if (obj->class_id <= V_CLASS_ID_MAX) {
                db->singleton[obj->class_id] = NULL;
            }
        } else if (obj->hash_key != NULL && obj->class_id <= V_CLASS_ID_MAX &&
                   db->keyed[obj->class_id] != NULL) {
            (void)vds_hash_del(db->keyed[obj->class_id], obj->hash_key, obj->hash_len);
        }
        drop = 1;
    }
    spin_unlock(&db->gate);
    if (drop) {
        object_destroy(obj);
    }
}

/**
 * @brief Unpin every object held by the current attempt.
 * @param[in] db Argument db
 */
static void unpin_all(struct vedam_lockdb *db)
{
    while (tls_pins != NULL) {
        unpin_obj(db, tls_pins->obj);
    }
}

/**
 * @brief Allocate a lock-map object and its existence lock.
 * @param[in] db Argument db
 * @param[in] class_id Argument class_id
 * @param[in] is_singleton Argument is_singleton
 * @param[in] hash_key Argument hash_key
 * @param[in] hash_len Argument hash_len
 * @return Result of alloc_object.
 */
static struct vedam_lock_object *alloc_object(struct vedam_lockdb *db, v_class_id_t class_id,
                                              int is_singleton, const unsigned char *hash_key,
                                              vuint32_t hash_len)
{
    struct vedam_lock_object *obj;
    v_status_t rc;

    obj = vplatform_calloc(1, sizeof(*obj));
    if (obj == NULL) {
        return NULL;
    }
    atomic_init(&obj->gate, 0);
    atomic_init(&obj->ref, 1);
    obj->class_id = class_id;
    obj->is_singleton = is_singleton;
    rc = vedam_lock_create(&obj->existence, db->group);
    if (rc != V_STATUS_SUCCESS) {
        vplatform_free(obj);
        return NULL;
    }
    if (hash_len > 0) {
        obj->hash_key = vplatform_calloc(1, hash_len);
        if (obj->hash_key == NULL) {
            object_destroy(obj);
            return NULL;
        }
        vplatform_memcpy(obj->hash_key, hash_key, hash_len);
        obj->hash_len = hash_len;
    }
    return obj;
}

/**
 * @brief Publish a pin under the database gate.
 * @param[in] db Argument db
 * @param[in] obj Argument obj
 * @param[in] err Argument err
 * @return Result of finish_pin.
 */
static int finish_pin(struct vedam_lockdb *db, struct vedam_lock_object *obj, int *err)
{
    struct pin *pin;

    if (find_pin(obj) != NULL) {
        atomic_fetch_sub_explicit(&obj->ref, 1, memory_order_relaxed);
        return 0;
    }
    pin = vplatform_calloc(1, sizeof(*pin));
    if (pin == NULL) {
        unpin_obj(db, obj);
        *err = (int)V_STATUS_NO_MEMORY;
        return -1;
    }
    pin->obj = obj;
    pin->next = tls_pins;
    tls_pins = pin;
    return 0;
}

/**
 * @brief Pin the singleton object for @p class_id.
 * @param[in] db Argument db
 * @param[in] class_id Argument class_id
 * @param[in] err Argument err
 * @return Result of pin_singleton.
 */
static struct vedam_lock_object *pin_singleton(struct vedam_lockdb *db, v_class_id_t class_id,
                                               int *err)
{
    struct vedam_lock_object *spare = NULL;
    struct vedam_lock_object *obj = NULL;

    *err = 0;

    spin_lock(&db->gate);
    obj = db->singleton[class_id];
    if (obj != NULL) {
        atomic_fetch_add_explicit(&obj->ref, 1, memory_order_relaxed);
        spin_unlock(&db->gate);
        if (finish_pin(db, obj, err) != 0) {
            return NULL;
        }
        return obj;
    }
    spin_unlock(&db->gate);

    spare = alloc_object(db, class_id, 1, NULL, 0);
    if (spare == NULL) {
        *err = (int)V_STATUS_NO_MEMORY;
        return NULL;
    }

    spin_lock(&db->gate);
    obj = db->singleton[class_id];
    if (obj != NULL) {
        atomic_fetch_add_explicit(&obj->ref, 1, memory_order_relaxed);
        spin_unlock(&db->gate);
        object_destroy(spare);
        if (finish_pin(db, obj, err) != 0) {
            return NULL;
        }
        return obj;
    }
    db->singleton[class_id] = spare;
    spare->next = db->all;
    db->all = spare;
    obj = spare;
    atomic_fetch_add_explicit(&db->miss_count, 1, memory_order_relaxed);
    spin_unlock(&db->gate);
    if (finish_pin(db, obj, err) != 0) {
        return NULL;
    }
    return obj;
}

/**
 * @brief Pin a keyed object for @p class_id.
 * @param[in] db Argument db
 * @param[in] class_id Argument class_id
 * @param[in] key Argument key
 * @param[in] key_len Argument key_len
 * @param[in] err Argument err
 * @return Result of pin_keyed.
 */
static struct vedam_lock_object *pin_keyed(struct vedam_lockdb *db, v_class_id_t class_id,
                                           const void *key, vuint32_t key_len, int *err)
{
    unsigned char stack[264];
    unsigned char *hash_key = NULL;
    struct vds_hash_node *node = NULL;
    struct vds_list *fresh = NULL;
    struct vedam_lock_object *spare = NULL;
    struct vedam_lock_object *obj = NULL;
    vuint32_t hash_len = 0;
    vuint32_t fresh_n = 0;
    void *found = NULL;
    int key_rc;
    v_status_t hash_rc;
    vds_hash_t *table;

    *err = 0;
    if (key_len == 0) {
        *err = (int)V_STATUS_INVALID_PARAMETER;
        return NULL;
    }
    if (class_id > V_CLASS_ID_MAX || db->keyed[class_id] == NULL) {
        *err = (int)V_STATUS_INVALID_PARAMETER;
        return NULL;
    }
    table = db->keyed[class_id];
    key_rc = copy_object_key(key, key_len, &hash_key, &hash_len, stack, sizeof(stack));
    if (key_rc == -1) {
        *err = (int)V_STATUS_INVALID_PARAMETER;
        return NULL;
    }
    if (key_rc != 0) {
        *err = (int)V_STATUS_NO_MEMORY;
        return NULL;
    }

    spin_lock(&db->gate);
    hash_rc = vds_hash_get(table, hash_key, hash_len, &found);
    if (hash_rc == V_STATUS_SUCCESS) {
        obj = (struct vedam_lock_object *)found;
        atomic_fetch_add_explicit(&obj->ref, 1, memory_order_relaxed);
        spin_unlock(&db->gate);
        if (hash_key != stack) {
            vplatform_free(hash_key);
        }
        if (finish_pin(db, obj, err) != 0) {
            return NULL;
        }
        return obj;
    }
    spin_unlock(&db->gate);

    spare = alloc_object(db, class_id, 0, hash_key, hash_len);
    if (spare == NULL) {
        *err = (int)V_STATUS_NO_MEMORY;
        goto out_key;
    }
    node = vds_hash_node_make(spare->hash_key, hash_len, spare);
    if (node == NULL) {
        *err = (int)V_STATUS_NO_MEMORY;
        goto out_spare;
    }

    for (;;) {
        vuint32_t nbucket;
        vuint32_t count;
        int room;

        spin_lock(&db->gate);
        hash_rc = vds_hash_get(table, hash_key, hash_len, &found);
        if (hash_rc == V_STATUS_SUCCESS) {
            obj = (struct vedam_lock_object *)found;
            atomic_fetch_add_explicit(&obj->ref, 1, memory_order_relaxed);
            spin_unlock(&db->gate);
            vplatform_free(node);
            node = NULL;
            object_destroy(spare);
            spare = NULL;
            break;
        }
        count = table->count;
        nbucket = table->nbucket;
        room = count + 1 <= nbucket;
        if (room) {
            hash_rc = vds_hash_insert(table, node);
            if (hash_rc == V_STATUS_SUCCESS) {
                node = NULL;
                spare->next = db->all;
                db->all = spare;
                obj = spare;
                spare = NULL;
                atomic_fetch_add_explicit(&db->miss_count, 1, memory_order_relaxed);
                spin_unlock(&db->gate);
                break;
            }
        }
        spin_unlock(&db->gate);

        if (fresh != NULL && fresh_n <= nbucket) {
            vds_hash_buckets_free(fresh);
            fresh = NULL;
            fresh_n = 0;
        }
        if (fresh == NULL) {
            if (nbucket > 0x7fffffffu) {
                *err = (int)V_STATUS_NO_MEMORY;
                goto out_spare;
            }
            fresh_n = nbucket << 1;
            fresh = vds_hash_prepare(fresh_n);
            if (fresh == NULL) {
                *err = (int)V_STATUS_NO_MEMORY;
                goto out_spare;
            }
        }

        spin_lock(&db->gate);
        if (table->count + 1 > table->nbucket && table->nbucket < fresh_n) {
            struct vds_list *old;
            vuint32_t old_n = 0;

            old = vds_hash_swap(table, fresh, fresh_n, &old_n);
            fresh = NULL;
            spin_unlock(&db->gate);
            vds_hash_buckets_free(old);
        } else {
            spin_unlock(&db->gate);
        }
    }
    if (hash_key != stack) {
        vplatform_free(hash_key);
        hash_key = NULL;
    }
    vds_hash_buckets_free(fresh);
    fresh = NULL;
    if (finish_pin(db, obj, err) != 0) {
        return NULL;
    }
    return obj;

out_spare:
    vplatform_free(node);
    object_destroy(spare);
    vds_hash_buckets_free(fresh);
out_key:
    if (hash_key != stack) {
        vplatform_free(hash_key);
    }
    return NULL;
}

/**
 * @brief Pin the object named by @p class_id and @p key.
 * @param[in] db Argument db
 * @param[in] class_id Argument class_id
 * @param[in] key Argument key
 * @param[in] key_len Argument key_len
 * @param[in] err Argument err
 * @return Result of pin_object.
 */
static struct vedam_lock_object *pin_object(struct vedam_lockdb *db, v_class_id_t class_id,
                                            const void *key, vuint32_t key_len, int *err)
{
    int singleton;

    *err = 0;
    if (class_id == V_CLASS_UNSPECIFIED || class_id > V_CLASS_ID_MAX) {
        *err = (int)V_STATUS_INVALID_PARAMETER;
        return NULL;
    }
    singleton = vedam_lock_class_singleton(class_id);
    if (singleton < 0) {
        *err = (int)V_STATUS_INVALID_PARAMETER;
        return NULL;
    }
    if (singleton) {
        if (key_len != 0) {
            *err = (int)V_STATUS_INVALID_PARAMETER;
            return NULL;
        }
        return pin_singleton(db, class_id, err);
    }
    return pin_keyed(db, class_id, key, key_len, err);
}

/**
 * @brief Grow the attribute-lock array until @p id fits.
 * @param[in] db Argument db
 * @param[in] obj Argument obj
 * @param[in] id Argument id
 * @return Result of ensure_attr.
 */
static vedam_lock_t *ensure_attr(struct vedam_lockdb *db, struct vedam_lock_object *obj,
                                 v_attr_id_t id)
{
    vedam_lock_t **grown = NULL;
    vedam_lock_t **old = NULL;
    vedam_lock_t *lock = NULL;
    vedam_lock_t *extra = NULL;
    v_status_t rc;
    vuint32_t cap;
    vuint32_t i;

    if (id == 0 || id >= VEDAM_LOCK_ATTR_MAX) {
        return NULL;
    }

    spin_lock(&obj->gate);
    if (id < obj->attr_cap && obj->attrs[id] != NULL) {
        lock = obj->attrs[id];
        spin_unlock(&obj->gate);
        return lock;
    }
    cap = obj->attr_cap;
    spin_unlock(&obj->gate);

    rc = vedam_lock_create(&lock, db->group);
    if (rc != V_STATUS_SUCCESS) {
        return NULL;
    }
    if (id >= cap) {
        cap = cap == 0 ? 8u : cap;
        while (cap <= id && cap < VEDAM_LOCK_ATTR_MAX) {
            cap *= 2u;
        }
        if (cap <= id) {
            (void)vedam_lock_destroy(lock);
            return NULL;
        }
        grown = vplatform_calloc(cap, sizeof(*grown));
        if (grown == NULL) {
            (void)vedam_lock_destroy(lock);
            return NULL;
        }
    }

    spin_lock(&obj->gate);
    if (id < obj->attr_cap && obj->attrs[id] != NULL) {
        extra = lock;
        lock = obj->attrs[id];
    } else {
        if (id >= obj->attr_cap) {
            for (i = 0; i < obj->attr_cap; i++) {
                grown[i] = obj->attrs[i];
            }
            old = obj->attrs;
            obj->attrs = grown;
            obj->attr_cap = cap;
            grown = NULL;
        }
        if (obj->attrs[id] == NULL) {
            obj->attrs[id] = lock;
        } else {
            extra = lock;
            lock = obj->attrs[id];
        }
    }
    spin_unlock(&obj->gate);
    vplatform_free(old);
    vplatform_free(grown);
    if (extra != NULL) {
        (void)vedam_lock_destroy(extra);
    }
    return lock;
}

/**
 * @brief Sort attribute ids for a stable lock order.
 * @param[in] ids Argument ids
 * @param[in] n Argument n
 */
static void sort_ids(v_attr_id_t *ids, vuint32_t n)
{
    vuint32_t i;
    vuint32_t j;

    for (i = 1; i < n; i++) {
        v_attr_id_t id = ids[i];

        j = i;
        while (j > 0 && ids[j - 1] > id) {
            ids[j] = ids[j - 1];
            j--;
        }
        ids[j] = id;
    }
}

/**
 * @brief Return non-zero when @p lock is already held by this attempt.
 * @param[in] rc Argument rc
 * @return Result of lock_held.
 */
static int lock_held(int rc)
{
    return rc == VEDAM_LOCK_HELD || rc == VEDAM_LOCK_JOINED;
}

/**
 * @brief Acquire the existence and attribute locks for one call.
 * @param[in] args Argument args
 * @return Result of take_locks.
 */
static int take_locks(struct invoke_args *args)
{
    v_attr_id_t stack_ids[VEDAM_LOCK_ID_STACK];
    v_attr_id_t *ids = stack_ids;
    vuint32_t n = 0;
    vuint32_t i;
    int err = 0;
    int rc;

    args->early = 0;
    args->obj = pin_object(args->db, args->class_id, args->key, args->key_len, &err);
    if (args->obj == NULL) {
        return err;
    }
    rc = vedam_lock_acquire(args->obj->existence, exist_exclusive(args->verb));
    if (!lock_held(rc)) {
        return rc;
    }
    if (args->obj->dead && rc == VEDAM_LOCK_HELD) {
        vedam_lock_release(args->obj->existence);
        unpin_obj(args->db, args->obj);
        args->obj = NULL;
        if (args->verb == V_VERB_CREATE) {
            args->obj = pin_object(args->db, args->class_id, args->key, args->key_len, &err);
            if (args->obj == NULL) {
                return err;
            }
            rc = vedam_lock_acquire(args->obj->existence, 1);
            if (!lock_held(rc)) {
                return rc;
            }
            if (args->obj->dead) {
                vedam_lock_release(args->obj->existence);
                return (int)V_STATUS_OBJECT_IN_USE;
            }
            return 0;
        }
        args->early = 1;
        args->early_status = V_STATUS_ITEM_NOT_FOUND;
        return 0;
    }
    if (args->verb == V_VERB_CREATE) {
        return 0;
    }
    if (args->verb == V_VERB_DELETE) {
        vedam_lock_t *snap[VEDAM_LOCK_ATTR_MAX];
        vuint32_t cap;
        vuint32_t count = 0;

        spin_lock(&args->obj->gate);
        cap = args->obj->attr_cap;
        if (cap > VEDAM_LOCK_ATTR_MAX) {
            cap = VEDAM_LOCK_ATTR_MAX;
        }
        for (i = 0; i < cap; i++) {
            if (args->obj->attrs[i] != NULL) {
                snap[count++] = args->obj->attrs[i];
            }
        }
        spin_unlock(&args->obj->gate);
        for (i = 0; i < count; i++) {
            rc = vedam_lock_acquire(snap[i], 1);
            if (!lock_held(rc)) {
                return rc;
            }
        }
        return 0;
    }

    if (args->attr_count > VEDAM_LOCK_LIST_MAX) {
        return (int)V_STATUS_INVALID_PARAMETER;
    }
    if (args->attr_count > 0 && args->attr_list == NULL) {
        return (int)V_STATUS_INVALID_PARAMETER;
    }
    if (args->attr_count > VEDAM_LOCK_ID_STACK) {
        ids = vplatform_calloc(args->attr_count, sizeof(*ids));
        if (ids == NULL) {
            return (int)V_STATUS_NO_MEMORY;
        }
    }
    {
        v_status_t first = V_STATUS_SUCCESS;

        for (i = 0; i < args->attr_count; i++) {
            v_attr_id_t id = args->attr_list[i].id;
            v_status_t st = V_STATUS_SUCCESS;

            if (id == 0) {
                if (args->status_list != NULL) {
                    args->status_list[i] = V_STATUS_SUCCESS;
                }
                continue;
            }
            if (id >= VEDAM_LOCK_ATTR_MAX || !vedam_lock_attr_known(args->class_id, id)) {
                st = (v_status_t)(V_STATUS_UNKNOWN_ATTRIBUTE_0 - (vint32_t)i);
            }
            if (args->status_list != NULL) {
                args->status_list[i] = st;
            }
            if (st != V_STATUS_SUCCESS && first == V_STATUS_SUCCESS) {
                first = st;
            }
            if (st == V_STATUS_SUCCESS) {
                ids[n++] = id;
            }
        }
        if (first != V_STATUS_SUCCESS) {
            if (ids != stack_ids) {
                vplatform_free(ids);
            }
            return (int)first;
        }
    }
    sort_ids(ids, n);
    rc = 0;
    for (i = 0; i < n; i++) {
        vedam_lock_t *lock;

        if (i > 0 && ids[i] == ids[i - 1]) {
            continue;
        }
        lock = ensure_attr(args->db, args->obj, ids[i]);
        if (lock == NULL) {
            rc = (int)V_STATUS_NO_MEMORY;
            break;
        }
        rc = vedam_lock_acquire(lock, attr_exclusive(args->verb));
        if (!lock_held(rc)) {
            break;
        }
        rc = 0;
    }
    if (ids != stack_ids) {
        vplatform_free(ids);
    }
    return rc;
}

/**
 * @brief Pin, take locks, and run the body once.
 *
 * A body that returns VEDAM_LOCK_RESTART ends this attempt so the outer
 * loop can begin it again.
 * @param[in] args Argument args
 * @return Result of attempt.
 */
static int attempt(struct invoke_args *args)
{
    int rc;
    v_status_t st;

    tls_depth++;
    rc = take_locks(args);
    if (rc == VEDAM_LOCK_RETRY) {
        tls_depth--;
        return VEDAM_LOCK_RESTART;
    }
    if (rc != 0) {
        vedam_lock_release_attempt();
        unpin_all(args->db);
        tls_depth--;
        return rc;
    }
    if (args->early) {
        vedam_lock_release_attempt();
        unpin_all(args->db);
        tls_depth--;
        return (int)args->early_status;
    }
    st = args->body(args->user);
    if ((int)st == VEDAM_LOCK_RESTART) {
        tls_depth--;
        return VEDAM_LOCK_RESTART;
    }
    if (args->verb == V_VERB_DELETE && st == V_STATUS_SUCCESS && args->obj != NULL) {
        args->obj->dead = 1;
    }
    vedam_lock_release_attempt();
    unpin_all(args->db);
    tls_depth--;
    return (int)st;
}

/**
 * @brief Take this call's locks on the attempt already running on this thread.
 *
 * VEDAM_LOCK_RETRY means this attempt must die. The caller returns
 * VEDAM_LOCK_RESTART so the outer loop drops every lock and starts over.
 * @param[in] args Argument args
 * @return Result of nested_invoke.
 */
static v_status_t nested_invoke(struct invoke_args *args)
{
    int rc;
    v_status_t st;

    rc = take_locks(args);
    if (rc == VEDAM_LOCK_RETRY) {
        return (v_status_t)VEDAM_LOCK_RESTART;
    }
    if (rc != 0) {
        return (v_status_t)rc;
    }
    if (args->early) {
        return args->early_status;
    }
    st = args->body(args->user);
    return st;
}

/**
 * @brief Create the process lock map on @p node.
 * @param[in] node Argument node
 * @return Result of vedam_lockdb_open.
 */
v_status_t vedam_lockdb_open(vedam_t *node)
{
    struct vedam_lockdb *db;
    v_status_t rc;
    v_class_id_t id;

    if (node == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    db = vplatform_calloc(1, sizeof(*db));
    if (db == NULL) {
        return V_STATUS_NO_MEMORY;
    }
    rc = vedam_lock_group_open(&db->group);
    if (rc != V_STATUS_SUCCESS) {
        vplatform_free(db);
        return rc;
    }
    atomic_init(&db->gate, 0);
    atomic_init(&db->miss_count, 0);
    for (id = (v_class_id_t)1; id <= V_CLASS_ID_MAX; id++) {
        if (vedam_lock_class_singleton(id) == 0) {
            rc = vds_hash_create(&db->keyed[id], 32);
            if (rc != V_STATUS_SUCCESS) {
                while (id > 1) {
                    id--;
                    if (db->keyed[id] != NULL) {
                        vds_hash_destroy(db->keyed[id]);
                        db->keyed[id] = NULL;
                    }
                }
                vedam_lock_group_close(db->group);
                vplatform_free(db);
                return rc;
            }
        }
    }
    node->locks = db;
    return V_STATUS_SUCCESS;
}

/**
 * @brief Free every object lock record on @p node.
 * @param[in] node Argument node
 */
void vedam_lockdb_close(vedam_t *node)
{
    struct vedam_lockdb *db;
    struct vedam_lock_object *obj;
    v_class_id_t id;

    if (node == NULL || node->locks == NULL) {
        return;
    }
    db = node->locks;
    node->locks = NULL;
    for (id = (v_class_id_t)1; id <= V_CLASS_ID_MAX; id++) {
        if (db->keyed[id] != NULL) {
            vds_hash_destroy(db->keyed[id]);
            db->keyed[id] = NULL;
        }
    }
    obj = db->all;
    while (obj != NULL) {
        struct vedam_lock_object *next = obj->next;

        object_destroy(obj);
        obj = next;
    }
    vedam_lock_group_close(db->group);
    vplatform_free(db);
}

/**
 * @brief Hold the locks for one call, run @p body, and release them at depth zero.
 *
 * The first call on this thread starts a Wait-Die attempt and repeats it
 * when an inner call dies. A call from inside @p body joins that attempt
 * and takes the locks of the other object. A queue or a network wait does
 * not belong inside the attempt.
 *
 * @param[in] class_id Argument class_id
 * @param[in] key Argument key
 * @param[in] key_len Argument key_len
 * @param[in] verb Argument verb
 * @param[in] origin Argument origin
 * @param[in] attr_count Argument attr_count
 * @param[in] attr_list Argument attr_list
 * @param[in] status_list Argument status_list
 * @param[in] body Argument body
 * @param[in] user Argument user
 * @return Result of vedam_lock_invoke.
 */
v_status_t vedam_lock_invoke(v_class_id_t class_id, const void *key, vuint32_t key_len,
                             v_verb_t verb, v_origin_t origin, vuint32_t attr_count,
                             v_attr_t *attr_list, v_status_t *status_list, vedam_lock_body_fn body,
                             void *user)
{
    vedam_t *node = vedam_node();
    struct invoke_args args;
    int rc;

    (void)origin;
    if (body == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (!locks_verb(verb) || node == NULL || node->locks == NULL) {
        return body(user);
    }
    vplatform_memset(&args, 0, sizeof(args));
    args.db = node->locks;
    args.class_id = class_id;
    args.key = key;
    args.key_len = key_len;
    args.verb = verb;
    args.attr_count = attr_count;
    args.attr_list = attr_list;
    args.status_list = status_list;
    args.body = body;
    args.user = user;
    if (tls_depth == 0) {
        do {
            rc = attempt(&args);
        } while (rc == VEDAM_LOCK_RESTART);
        return (v_status_t)rc;
    }
    return nested_invoke(&args);
}

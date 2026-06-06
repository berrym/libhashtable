/**
 * @file ht_strset.c
 * @brief Type-wrapped string set (presence-only).
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 *
 * A set of strings: membership without an associated value. Keys are stored
 * with a NULL value, so no value storage or callbacks are needed.
 */

#include "ht.h"

/// Wrapper around ht_create that creates a string set.
ht_strset_t *ht_strset_create(const ht_str_options_t *opts) {
    const ht_str_options_t o = opts ? *opts : (ht_str_options_t){0};
    if (o.case_insensitive && o.flooding_resistant) {
        return NULL;
    }

    /// Keys are copied; the set stores no values.
    const ht_callbacks_t callbacks = {str_copy, str_free, NULL, NULL};

    const ht_options_t base = {
        .hash = o.flooding_resistant ? ht_hash_siphash
                : o.case_insensitive ? ht_hash_fnv1a_casecmp
                                     : ht_hash_fnv1a,
        .keyeq = o.case_insensitive ? str_caseeq : str_eq,
        .keylen = str_len,
        .callbacks = callbacks,
        .key_mode = o.flooding_resistant ? HT_KEY_RANDOM : HT_KEY_NONE,
        .key_best_effort = o.best_effort,
        .initial_capacity = o.initial_capacity,
    };

    return (ht_strset_t *)ht_create(&base);
}

/// Wrapper around ht_destroy for a string set.
void ht_strset_destroy(ht_strset_t *ht) { ht_destroy((ht_t *)ht); }

/// Add a member. Returns true if the member was newly added and false if it
/// was already present.
bool ht_strset_add(ht_strset_t *ht, const char *key) {
    return ht_upsert((ht_t *)ht, (void *)key, NULL);
}

/// Remove a member.
void ht_strset_remove(ht_strset_t *ht, const char *key) {
    ht_remove((ht_t *)ht, (void *)key);
}

/// Return whether a member is present.
bool ht_strset_contains(ht_strset_t *ht, const char *key) {
    return ht_contains((ht_t *)ht, (void *)key);
}

/// Wrapper around ht_enum_create for a string set.
ht_enum_t *ht_strset_enum_create(ht_strset_t *ht) {
    return ht_enum_create((ht_t *)ht);
}

/// Return the next member. There is no value.
bool ht_strset_enum_next(ht_enum_t *he, const char **key) {
    const void *val = NULL;
    return ht_enum_next(he, (const void **)key, &val);
}

/// Wrapper around ht_enum_destroy for a string-set enumeration object.
void ht_strset_enum_destroy(ht_enum_t *he) { ht_enum_destroy(he); }

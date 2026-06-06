/* ht_strptr.c - Type-wrapped string->pointer hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2024
 *
 * Maps a string key to an opaque, caller-owned pointer value, stored directly
 * rather than smuggled through a string representation. The table copies the
 * key; it stores the value pointer as given and never frees the pointed-to
 * object.
 */

#include "ht.h"

#include <stdlib.h>
#include <string.h>

/**
 * ht_strptr_create:
 *      Wrapper around ht_create that creates a string->pointer hash table.
 */
ht_strptr_t *ht_strptr_create(const ht_str_options_t *opts) {
    const ht_str_options_t o = opts ? *opts : (ht_str_options_t){0};
    if (o.case_insensitive && o.flooding_resistant) {
        return NULL;
    }

    /// Keys are copied; values are stored as given (passthrough copy/free).
    const ht_callbacks_t callbacks = {(void *(*)(const void *))strdup,
                                      (void (*)(const void *))free, NULL, NULL};

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

    return (ht_strptr_t *)ht_create(&base);
}

/**
 * ht_strptr_destroy:
 *      Wrapper around ht_destroy for a string->pointer hash table.
 */
void ht_strptr_destroy(ht_strptr_t *ht) { ht_destroy((ht_t *)ht); }

/**
 * ht_strptr_insert:
 *      Insert a string key mapping to a caller-owned pointer value.
 */
void ht_strptr_insert(ht_strptr_t *ht, const char *key, void *val) {
    ht_insert((ht_t *)ht, (void *)key, val);
}

/**
 * ht_strptr_remove:
 *      Remove the entry for a string key.
 */
void ht_strptr_remove(ht_strptr_t *ht, const char *key) {
    ht_remove((ht_t *)ht, (void *)key);
}

/**
 * ht_strptr_get:
 *      Return the pointer stored under a string key, or NULL if absent.
 */
void *ht_strptr_get(ht_strptr_t *ht, const char *key) {
    return ht_get((ht_t *)ht, (void *)key);
}

/**
 * ht_strptr_enum_create:
 *      Wrapper around ht_enum_create for a string->pointer hash table.
 */
ht_enum_t *ht_strptr_enum_create(ht_strptr_t *ht) {
    return ht_enum_create((ht_t *)ht);
}

/**
 * ht_strptr_enum_next:
 *      Return the next key/value pair. The value aliases the stored pointer.
 */
bool ht_strptr_enum_next(ht_enum_t *he, const char **key, void **val) {
    return ht_enum_next(he, (const void **)key, (const void **)val);
}

/**
 * ht_strptr_enum_destroy:
 *      Wrapper around ht_enum_destroy for a string->pointer enumeration object.
 */
void ht_strptr_enum_destroy(ht_enum_t *he) { ht_enum_destroy(he); }

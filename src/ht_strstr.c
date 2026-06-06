/* ht_strstr.c - Type wrapped implementation of a string->string hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2024
 */

#include "ht.h"

#include <stdlib.h>
#include <string.h>

/**
 * ht_strstr_create:
 *      Wrapper aroung ht_create that creates a string->string hash table.
 */
ht_strstr_t *ht_strstr_create(const ht_str_options_t *opts) {
    const ht_str_options_t o = opts ? *opts : (ht_str_options_t){0};
    if (o.case_insensitive && o.flooding_resistant) {
        return NULL;
    }

    const ht_callbacks_t callbacks = {
        (void *(*)(const void *))strdup, (void (*)(const void *))free,
        (void *(*)(const void *))strdup, (void (*)(const void *))free};

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

    return (ht_strstr_t *)ht_create(&base);
}

/**
 * ht_strstr_destroy:
 *      Wrapper around ht_destroy that destroys a string->string hash table.
 */
void ht_strstr_destroy(ht_strstr_t *ht) { ht_destroy((ht_t *)ht); }

/**
 * ht_strstr_insert:
 *      Wrapper around ht_insert that inserts a string->string key value pair
 * into a hash table.
 */
void ht_strstr_insert(ht_strstr_t *ht, const char *key, const char *val) {
    ht_insert((ht_t *)ht, key, val);
}

/**
 * ht_strstr_remove:
 *      Wrapper around ht_remove that removes a bucket from a string->string
 * hash table.
 */
void ht_strstr_remove(ht_strstr_t *ht, const char *key) {
    ht_remove((ht_t *)ht, (void *)key);
}

/**
 * ht_strstr_get:
 *      Wrapper around ht_get for string->string hash table.
 */
const char *ht_strstr_get(ht_strstr_t *ht, const char *key) {
    return ht_get((ht_t *)ht, (void *)key);
}

/**
 * ht_strstr_enum_create:
 *      Wrapper around ht_enum_create the makes an enumeration object for
 * string->string hash table.
 */
ht_enum_t *ht_strstr_enum_create(ht_strstr_t *ht) {
    return (ht_enum_t *)ht_enum_create((ht_t *)ht);
}

/**
 * ht_strtr_enum_next:
 *      Wrapper around ht_enum_next that returns the next bucket contents of a
 * string->string hash table.
 */
bool ht_strstr_enum_next(ht_enum_t *he, const char **key, const char **val) {
    return ht_enum_next((ht_enum_t *)he, (const void **)key,
                        (const void **)val);
}

/**
 * ht_strstr_enum_destroy:
 *      Wrapper around ht_enum_destroy that destroys a string->string hash table
 * enumeration object.
 */
void ht_strstr_enum_destroy(ht_enum_t *he) { ht_enum_destroy((ht_enum_t *)he); }

/* ht_strdouble.c - Type wrapped implementation of a string->double hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2024
 */

#include "ht.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * __doubledup:
 *      Create a pointer duplicating val.
 */
static double *__doubledup(const double *val) {
    double *d = calloc(1, sizeof(double));
    if (!d) {
        perror("__floatdup");
        return NULL;
    }

    return memcpy(d, val, sizeof(double));
}

/**
 * ht_strdouble_create:
 *      Wrapper aroung ht_create that creates a string->double hash table.
 */
ht_strdouble_t *ht_strdouble_create(const ht_str_options_t *opts) {
    const ht_str_options_t o = opts ? *opts : (ht_str_options_t){0};
    if (o.case_insensitive && o.flooding_resistant) {
        return NULL;
    }

    const ht_callbacks_t callbacks = {
        (void *(*)(const void *))strdup, (void (*)(const void *))free,
        (void *(*)(const void *))__doubledup, (void (*)(const void *))free};

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

    return (ht_strdouble_t *)ht_create(&base);
}

/**
 * ht_strdouble_destroy:
 *      Wrapper around ht_destroy that destroys a string->double hash table.
 */
void ht_strdouble_destroy(ht_strdouble_t *ht) { ht_destroy((ht_t *)ht); }

/**
 * ht_strdouble_insert:
 *      Wrapper around ht_insert that inserts a string->double key value pair
 * into a hash table.
 */
void ht_strdouble_insert(ht_strdouble_t *ht, const char *key,
                         const double *val) {
    ht_insert((ht_t *)ht, (void *)key, (void *)val);
}

/**
 * ht_strdouble_remove:
 *      Wrapper around ht_remove that removes a bucket from a string->double
 * hash table.
 */
void ht_strdouble_remove(ht_strdouble_t *ht, const char *key) {
    ht_remove((ht_t *)ht, (void *)key);
}

/**
 * ht_strdouble_get:
 *      Wrapper around ht_get for string->double hash table.
 */
void *ht_strdouble_get(ht_strdouble_t *ht, const char *key) {
    return ht_get((ht_t *)ht, (void *)key);
}

/**
 * ht_strdouble_enum_create:
 *      Wrapper around ht_enum_create the makes an enumeration object for
 * string->double hash table.
 */
ht_enum_t *ht_strdouble_enum_create(ht_strdouble_t *ht) {
    return ht_enum_create((ht_t *)ht);
}

/**
 * ht_strdouble_enum_next:
 *      Wrapper around ht_enum_next that returns the next bucket contents of a
 * string->double hash table.
 */
bool ht_strdouble_enum_next(ht_enum_t *he, const char **key,
                            const double **val) {
    return ht_enum_next(he, (const void **)key, (const void **)val);
}

/**
 * ht_strdouble_enum_destroy:
 *      Wrapper around ht_enum_destroy that destroys a string->double hash table
 * enumeration object.
 */
void ht_strdouble_enum_destroy(ht_enum_t *he) { ht_enum_destroy(he); }

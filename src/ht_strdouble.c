/**
 * @file ht_strdouble.c
 * @brief Type wrapped implementation of a string->double hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 */

#include "ht.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Create a pointer duplicating val (value-copy callback). The user_data
/// argument is unused.
static void *__doubledup(const void *val, void *user_data) {
    (void)user_data;
    double *d = calloc(1, sizeof(double));
    if (!d) {
        perror("__doubledup");
        return NULL;
    }

    return memcpy(d, val, sizeof(double));
}

/// Wrapper aroung ht_create that creates a string->double hash table.
ht_strdouble_t *ht_strdouble_create(const ht_str_options_t *opts) {
    const ht_str_options_t o = opts ? *opts : (ht_str_options_t){0};
    if (o.case_insensitive && o.flooding_resistant) {
        return NULL;
    }

    const ht_callbacks_t callbacks = {str_copy, str_free, __doubledup,
                                      str_free};

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

/// Wrapper around ht_destroy that destroys a string->double hash table.
void ht_strdouble_destroy(ht_strdouble_t *ht) { ht_destroy((ht_t *)ht); }

/// Wrapper around ht_insert that inserts a string->double key value pair
/// into a hash table.
void ht_strdouble_insert(ht_strdouble_t *ht, const char *key,
                         const double *val) {
    ht_insert((ht_t *)ht, (void *)key, (void *)val);
}

/// Wrapper around ht_remove that removes a bucket from a string->double
/// hash table.
void ht_strdouble_remove(ht_strdouble_t *ht, const char *key) {
    ht_remove((ht_t *)ht, (void *)key);
}

/// Wrapper around ht_get for string->double hash table.
void *ht_strdouble_get(ht_strdouble_t *ht, const char *key) {
    return ht_get((ht_t *)ht, (void *)key);
}

/// Wrapper around ht_enum_create the makes an enumeration object for
/// string->double hash table.
ht_enum_t *ht_strdouble_enum_create(ht_strdouble_t *ht) {
    return ht_enum_create((ht_t *)ht);
}

/// Wrapper around ht_enum_next that returns the next bucket contents of a
/// string->double hash table.
bool ht_strdouble_enum_next(ht_enum_t *he, const char **key,
                            const double **val) {
    return ht_enum_next(he, (const void **)key, (const void **)val);
}

/// Wrapper around ht_enum_destroy that destroys a string->double hash table
/// enumeration object.
void ht_strdouble_enum_destroy(ht_enum_t *he) { ht_enum_destroy(he); }

/* ht_strdouble.c - Type wrapped implementation of a string->double hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ht.h"

/**
 * __doubledup:
 *      Create a pointer duplicating val.
 */
static double *__doubledup(double *val)
{
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
ht_strdouble_t *ht_strdouble_create(unsigned int flags)
{
    ht_hash hash = fnv1a_hash_str;
    ht_keyeq keyeq = str_eq;
    ht_callbacks_t callbacks = {
        (void *(*)(void *))strdup,
        free,
        (void *(*)(void *))__doubledup,
        free
    };

    if (flags & HT_STR_CASECMP) {
        hash = fnv1a_hash_str_casecmp;
        keyeq = str_caseeq;
    }

    return (ht_strdouble_t *)ht_create(hash, keyeq, &callbacks, flags);
}

/**
 * ht_strdouble_destroy:
 *      Wrapper around ht_destroy that destroys a string->double hash table.
 */
void ht_strdouble_destroy(ht_strdouble_t *ht)
{
    ht_destroy((ht_t *)ht);
}

/**
 * ht_strdouble_insert:
 *      Wrapper around ht_insert that inserts a string->double key value pair into a hash table.
 */
void ht_strdouble_insert(ht_strdouble_t *ht, const char *key, const double *val)
{
    ht_insert((ht_t *)ht, (void *)key, (void *)val);
}

/**
 * ht_strdouble_remove:
 *      Wrapper around ht_remove that removes a bucket from a string->double hash table.
 */
void ht_strdouble_remove(ht_strdouble_t *ht, const char *key)
{
    ht_remove((ht_t *)ht, (void *)key);
}

/**
 * ht_strdouble_get:
 *      Wrapper around ht_get for string->double hash table.
 */
void *ht_strdouble_get(ht_strdouble_t *ht, const char *key)
{
    return ht_get((ht_t *)ht, (void *)key);
}

/**
 * ht_strdouble_enum_create:
 *      Wrapper around ht_enum_create the makes an enumeration object for string->double hash table.
 */
ht_enum_t *ht_strdouble_enum_create(ht_strdouble_t *ht)
{
    return ht_enum_create((ht_t *)ht);
}

/**
 * ht_strdouble_enum_next:
 *      Wrapper around ht_enum_next that returns the next bucket contents of a string->double hash table.
 */
bool ht_strdouble_enum_next(ht_enum_t *he, const char **key, const double **val)
{
    return ht_enum_next(he, (void **)key, (void **)val);
}

/**
 * ht_strdouble_enum_destroy:
 *      Wrapper around ht_enum_destroy that destroys a string->double hash table enumeration object.
 */
void ht_strdouble_enum_destroy(ht_enum_t *he)
{
    ht_enum_destroy(he);
}

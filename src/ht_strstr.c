/* ht_strstr.c - Type wrapped implementation of a string->string hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <stdlib.h>
#include <string.h>
#include "ht.h"

/**
 * ht_strstr_create:
 *      Wrapper aroung ht_create that creates a string->string hash table.
 */
ht_strstr_t *ht_strstr_create(unsigned int flags)
{
    ht_hash hash = fnv1a_hash_str;
    ht_keyeq keyeq = str_eq;
    const ht_callbacks_t callbacks = {
        (void *(*)(const void *))strdup,
        (void (*)(const void *))free,
        (void *(*)(const void *))strdup,
        (void (*)(const void *))free
    };

    if (flags & HT_STR_CASECMP) {
        hash = fnv1a_hash_str_casecmp;
        keyeq = str_caseeq;
    }

    return (ht_strstr_t *)ht_create(hash, keyeq, &callbacks, flags);
}

/**
 * ht_strstr_destroy:
 *      Wrapper around ht_destroy that destroys a string->string hash table.
 */
void ht_strstr_destroy(ht_strstr_t *ht)
{
    ht_destroy((ht_t *)ht);
}

/**
 * ht_strstr_insert:
 *      Wrapper around ht_insert that inserts a string->string key value pair into a hash table.
 */
void ht_strstr_insert(ht_strstr_t *ht, const char *key, const char *val)
{
    ht_insert((ht_t *)ht, key, val);
}

/**
 * ht_strstr_remove:
 *      Wrapper around ht_remove that removes a bucket from a string->string hash table.
 */
void ht_strstr_remove(ht_strstr_t *ht, const char *key)
{
    ht_remove((ht_t *)ht, (void *)key);
}

/**
 * ht_strstr_get:
 *      Wrapper around ht_get for string->string hash table.
 */
const char *ht_strstr_get(ht_strstr_t *ht, const char *key)
{
    return ht_get((ht_t *)ht, (void *)key);
}

/**
 * ht_strstr_enum_create:
 *      Wrapper around ht_enum_create the makes an enumeration object for string->string hash table.
 */
ht_enum_t *ht_strstr_enum_create(ht_strstr_t *ht)
{
    return (ht_enum_t *)ht_enum_create((ht_t *)ht);
}

/**
 * ht_strtr_enum_next:
 *      Wrapper around ht_enum_next that returns the next bucket contents of a string->string hash table.
 */
bool ht_strstr_enum_next(ht_enum_t *he, const char **key, const char **val)
{
    return ht_enum_next((ht_enum_t *)he, (const void **)key, (const void **)val);
}

/**
 * ht_strstr_enum_destroy:
 *      Wrapper around ht_enum_destroy that destroys a string->string hash table enumeration object.
 */
void ht_strstr_enum_destroy(ht_enum_t *he)
{
    ht_enum_destroy((ht_enum_t *)he);
}

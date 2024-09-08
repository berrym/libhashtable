/* ht_strint.c - Type wrapped implementation of a string->int hash table.
 *
 * Project: libashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2024
 */

#include "ht.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * __intdup:
 *      Create a pointer duplicating val.
 */
static int *__intdup(const int *val) {
    int *i = calloc(1, sizeof(int));
    if (!i) {
        perror("__intdup");
        return NULL;
    }

    return memcpy(i, val, sizeof(int));
}

/**
 * ht_strint_create:
 *      Wrapper aroung ht_create that creates a string->int hash table.
 */
ht_strint_t *ht_strint_create(unsigned int flags) {
    ht_hash hash = fnv1a_hash_str;
    ht_keyeq keyeq = str_eq;
    const ht_callbacks_t callbacks = {
        (void *(*)(const void *))strdup, (void (*)(const void *))free,
        (void *(*)(const void *))__intdup, (void (*)(const void *))free};

    if (flags & HT_STR_CASECMP) {
        hash = fnv1a_hash_str_casecmp;
        keyeq = str_caseeq;
    }

    return (ht_strint_t *)ht_create(hash, keyeq, &callbacks, flags);
}

/**
 * ht_strint_destroy:
 *      Wrapper around ht_destroy that destroys a string->int hash table.
 */
void ht_strint_destroy(ht_strint_t *ht) { ht_destroy((ht_t *)ht); }

/**
 * ht_strint_insert:
 *      Wrapper around ht_insert that inserts a string->int key value pair into
 * a hash table.
 */
void ht_strint_insert(ht_strint_t *ht, const char *key, const int *val) {
    ht_insert((ht_t *)ht, (void *)key, (void *)val);
}

/**
 * ht_strint_remove:
 *      Wrapper around ht_remove that removes a bucket from a string->int hash
 * table.
 */
void ht_strint_remove(ht_strint_t *ht, const char *key) {
    ht_remove((ht_t *)ht, (void *)key);
}

/**
 * ht_strint_get:
 *      Wrapper around ht_get for string->int hash table.
 */
void *ht_strint_get(ht_strint_t *ht, const char *key) {
    return ht_get((ht_t *)ht, (void *)key);
}

/**
 * ht_strint_enum_create:
 *      Wrapper around ht_enum_create the makes an enumeration object for
 * string->int hash table.
 */
ht_enum_t *ht_strint_enum_create(ht_strint_t *ht) {
    return ht_enum_create((ht_t *)ht);
}

/**
 * ht_strint_enum_next:
 *      Wrapper around ht_enum_next that returns the next bucket contents of a
 * string->int hash table.
 */
bool ht_strint_enum_next(ht_enum_t *he, const char **key, const int **val) {
    return ht_enum_next(he, (const void **)key, (const void **)val);
}

/**
 * ht_strint_enum_destroy:
 *      Wrapper around ht_enum_destroy that destroys a string->int hash table
 * enumeration object.
 */
void ht_strint_enum_destroy(ht_enum_t *he) { ht_enum_destroy(he); }

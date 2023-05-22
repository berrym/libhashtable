/* ht_strdouble.c - Type wrapped implementation of a string->double hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "ht.h"
#include "ht_strdouble.h"

#if defined(CPU_32_BIT)

#define FNV1A_PRIME (0x01000193)  // 16777619 (32 bit)

/**
 * __fnv1a_hash_str_int:
 *      Return a hash key using the 32 bit FNV1A algorithm.
 */
static uint32_t __fnv1a_hash_str_int(const void *key, uint32_t seed, bool ignore_case)
{
    uint32_t h, c;

    h = seed;

    for (unsigned char *p = (unsigned char *)key; *p; p++) {
        c = (uint32_t)(*p);
        if (ignore_case)
            c = tolower(c);
        h ^= (uint32_t)c;
        h *= FNV1A_PRIME;
    }

    return h;
}

/**
 * __fnv1a_hash_str:
 *      Wrapper around __fnv1a_hash_str_int that uses case sensitive keys.
 */
static uint32_t __fnv1a_hash_str(const void *key, uint32_t seed)
{
    return __fnv1a_hash_str_int(key, seed, false);
}

/**
 * __fnv1a_hash_str_casecmp:
 *      Wrapper around __fnv1a_hash_str_int that uses case insensitive keys.
 */
static uint32_t __fnv1a_hash_str_casecmp(const void *key, uint32_t seed)
{
    return __fnv1a_hash_str_int(key, seed, true);
}

#elif defined(CPU_64_BIT)

#define FNV1A_PRIME (0x00000100000001B3)  // 1099511628211 (64 bit)

/**
 * __fnv1a_hash_str_int:
 *      Return a hash key using the 64 bit FNV1A algorithm.
 */
static uint64_t __fnv1a_hash_str_int(const void *key, uint64_t seed, bool ignore_case)
{
    uint64_t h, c;

    h = seed;

    for (unsigned char *p = (unsigned char *)key; *p; p++) {
        c = (uint64_t)(*p);
        if (ignore_case)
            c = tolower(c);
        h ^= (uint64_t)c;
        h *= FNV1A_PRIME;
    }

    return h;
}

/**
 * __fnv1a_hash_str:
 *      Wrapper around __fnv1a_hash_str_int that uses case sensitive keys.
 */
static uint64_t __fnv1a_hash_str(const void *key, uint64_t seed)
{
    return __fnv1a_hash_str_int(key, seed, false);
}

/**
 * __fnv1a_hash_str_casecmp:
 *      Wrapper around __fnv1a_hash_str_int that uses case insensitive keys.
 */
static uint64_t __fnv1a_hash_str_casecmp(const void *key, uint64_t seed)
{
    return __fnv1a_hash_str_int(key, seed, false);
}

#endif

/**
 * __ht_str_eq:
 *      Case sensitive string comparison function.
 */
static bool __ht_str_eq(const void *a, const void *b)
{
    return (strcmp(a, b) == 0) ? true : false;
}

/**
 * __ht_str_caseeq:
 *      Case insensitive string comparison function.
 */
static bool __ht_str_caseeq(const void *a, const void *b)
{
    return (strcasecmp(a, b) == 0) ? true : false;
}

/**
 * __doubledup:
 *      Create a pointer duplicating val.
 */
static double *__doubledup(double *val)
{
    double *i = calloc(1, sizeof(double));
    if (!i) {
        perror("__floatdup");
        return NULL;
    }

    return memcpy(i, val, sizeof(double));
}

/**
 * ht_strdouble_create:
 *      Wrapper aroung ht_create that creates a string->int hash table.
 */
ht_strdouble_t *ht_strdouble_create(unsigned int flags)
{
    ht_hash hash = __fnv1a_hash_str;
    ht_keyeq keyeq = __ht_str_eq;
    ht_callbacks_t callbacks = {
        (void *(*)(void *))strdup,
        free,
        (void *(*)(void *))__doubledup,
        free
    };

    if (flags & HT_STR_CASECMP) {
        hash = __fnv1a_hash_str_casecmp;
        keyeq = __ht_str_caseeq;
    }

    return (ht_strdouble_t *)ht_create(hash, keyeq, &callbacks, flags);
}

/**
 * ht_strdouble_destroy:
 *      Wrapper around ht_destroy that destroys a string->int hash table.
 */
void ht_strdouble_destroy(ht_strdouble_t *ht)
{
    ht_destroy((ht_t *)ht);
}

/**
 * ht_strdouble_insert:
 *      Wrapper around ht_insert that inserts a string->int key value pair into a hash table.
 */
void ht_strdouble_insert(ht_strdouble_t *ht, const char *key, const double *val)
{
    ht_insert((ht_t *)ht, (void *)key, (void *)val);
}

/**
 * ht_strdouble_remove:
 *      Wrapper around ht_remove that removes a bucket from a string->int hash table.
 */
void ht_strdouble_remove(ht_strdouble_t *ht, const char *key)
{
    ht_remove((ht_t *)ht, (void *)key);
}

/**
 * ht_strdouble_get:
 *      Wrapper around ht_get for string->int hash table.
 */
bool ht_strdouble_get(ht_strdouble_t *ht, const char *key, const double **val)
{
    return ht_get((ht_t *)ht, (void *)key, (void **)val);
}

/**
 * ht_strdouble_get_direct:
 *      Wrapper around ht_get_direct for string->int hash table.
 */
void *ht_strdouble_get_direct(ht_strdouble_t *ht, const char *key)
{
    return ht_get_direct((ht_t *)ht, (void *)key);
}

/**
 * ht_strdouble_enum_create:
 *      Wrapper around ht_enum_create the makes an enumeration object for string->int hash table.
 */
ht_strdouble_enum_t *ht_strdouble_enum_create(ht_strdouble_t *ht)
{
    return (ht_strdouble_enum_t *)ht_enum_create((ht_t *)ht);
}

/**
 * ht_strdouble_enum_next:
 *      Wrapper around ht_enum_next that returns the next bucket contents of a string->int hash table.
 */
bool ht_strdouble_enum_next(ht_strdouble_enum_t *he, const char **key, const double **val)
{
    return ht_enum_next((ht_enum_t *)he, (void **)key, (void **)val);
}

/**
 * ht_strdouble_enum_destroy:
 *      Wrapper around ht_enum_destroy that destroys a string->int hash table enumeration object.
 */
void ht_strdouble_enum_destroy(ht_strdouble_enum_t *he)
{
    ht_enum_destroy((ht_enum_t *)he);
}

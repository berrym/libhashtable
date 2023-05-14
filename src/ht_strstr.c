/* ht_strstr.c - Type wrapped implementation of a string->string hash table.
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
#include <time.h>
#include "ht.h"
#include "ht_strstr.h"

#if defined(CPU_32_BIT)

#define FNV1A_OFFSET (0x811C9DC5) // 2166136261 (32 bit)
#define FNV1A_PRIME (0x01000193)  // 16777619 (32 bit)

/**
 * __fnv1a_hash_str_int:
 *      Return a hash using the 32 bit FNV1A algorithm.
 */
static uint32_t __fnv1a_hash_str_int(const void *key, bool ignore_case)
{
    uint32_t h, c;

    h = (uint32_t)time(NULL);
    h ^= ((uint32_t)ht_create << 16) | (uint32_t)&h;
    h ^= (uint32_t)&h;

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
static uint32_t __fnv1a_hash_str(const void *key)
{
    return __fnv1a_hash_str_int(key, false);
}

/**
 * __fnv1a_hash_str:
 *      Wrapper around __fnv1a_hash_str_int that uses case sensitive keys.
 */
static uint32_t __fnv1a_hash_str_casecmp(const void *key)
{
    return __fnv1a_hash_str_int(key, true);
}

#elif defined(CPU_64_BIT)

#define FNV1A_OFFSET (0xCBF29CE484222325) // 14695981039346656037 (64 bit)
#define FNV1A_PRIME (0x00000100000001B3)  // 1099511628211 (64 bit)

/**
 * __fnv1a_hash_str_int:
 *      Return a hash using the 64 bit FNV1A algorithm.
 */
static uint64_t __fnv1a_hash_str_int(const void *key, bool ignore_case)
{
    uint64_t h, c;

    h = (uint64_t)time(NULL);
    h ^= ((uint64_t)ht_create << 32) | (uint64_t)&h;
    h ^= (uint64_t)&h;

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
static uint64_t __fnv1a_hash_str(const void *key)
{
    return __fnv1a_hash_str_int(key, false);
}

/**
 * __fnv1a_hash_str_casecmp:
 *      Wrapper around __fnv1a_hash_str_int that uses case insensitive keys.
 */
static uint64_t __fnv1a_hash_str_casecmp(const void *key)
{
    return __fnv1a_hash_str_int(key, true);
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
 * ht_strstr_create:
 *      Wrapper aroung ht_create that creates a string->string hash table.
 */
ht_strstr_t *ht_strstr_create(unsigned int flags)
{
    ht_hash hash = __fnv1a_hash_str;
    ht_keyeq keyeq = __ht_str_eq;
    ht_callbacks_t callbacks = {
        (void *(*)(void *))strdup,
        free,
        (void *(*)(void *))strdup,
        free
    };

    if (flags & HTABLE_STR_CASECMP) {
        hash = __fnv1a_hash_str_casecmp;
        keyeq = __ht_str_caseeq;
    }

    return (ht_strstr_t *)ht_create(hash, keyeq, &callbacks);
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
    ht_insert((ht_t *)ht, (void *)key, (void *)val);
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
bool ht_strstr_get(ht_strstr_t *ht, const char *key, const char **val)
{
    return ht_get((ht_t *)ht, (void *)key, (void **)val);
}

/**
 * ht_strstr_get_direct:
 *      Wrapper around ht_get_direct for string->string hash table.
 */
const char *ht_strstr_get_direct(ht_strstr_t *ht, const char *key)
{
    return ht_get_direct((ht_t *)ht, (void *)key);
}

/**
 * ht_strstr_enum_create:
 *      Wrapper around ht_enum_create the makes an enumeration object for string->string hash table.
 */
ht_strstr_enum_t *ht_strstr_enum_create(ht_strstr_t *ht)
{
    return (ht_strstr_enum_t *)ht_enum_create((ht_t *)ht);
}

/**
 * ht_strtr_enum_next:
 *      Wrapper around ht_enum_next that returns the next bucket contents of a string->string hash table.
 */
bool ht_strstr_enum_next(ht_strstr_enum_t *he, const char **key, const char **val)
{
    return ht_enum_next((ht_enum_t *)he, (void **)key, (void **)val);
}

/**
 * ht_strstr_enum_destroy:
 *      Wrapper around ht_enum_destroy that destroys a string->string hash table enumeration object.
 */
void ht_strstr_enum_destroy(ht_strstr_enum_t *he)
{
    ht_enum_destroy((ht_enum_t *)he);
}

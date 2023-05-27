/* ht_fnv1a.c - FNV1A hash algorithm implementation.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "ht_fnv1a.h"

#if defined(CPU_32_BIT)

/**
 * __fnv1a_hash:
 *      Return a hash key using the 32 bit FNV1A algorithm.
 */
static uint32_t __fnv1a_hash(const void *key, uint32_t seed, bool ignore_case)
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
 * fnv1a_hash_str:
 *      Wrapper around __fnv1a_hash that uses case sensitive keys.
 */
uint32_t fnv1a_hash_str(const void *key, uint32_t seed)
{
    return __fnv1a_hash(key, seed, false);
}

/**
 * fnv1a_hash_str_casecmp:
 *      Wrapper around __fnv1a_hash that uses case insensitive keys.
 */
uint32_t fnv1a_hash_str_casecmp(const void *key, uint32_t seed)
{
    return __fnv1a_hash(key, seed, true);
}

#elif defined(CPU_64_BIT)

/**
 * __fnv1a_hash:
 *      Return a hash key using the 64 bit FNV1A algorithm.
 */
static uint64_t __fnv1a_hash(const void *key, uint64_t seed, bool ignore_case)
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
 * fnv1a_hash_str:
 *      Wrapper around __fnv1a_hash that uses case sensitive keys.
 */
uint64_t fnv1a_hash_str(const void *key, uint64_t seed)
{
    return __fnv1a_hash(key, seed, false);
}

/**
 * fnv1a_hash_str_casecmp:
 *      Wrapper around __fnv1a_hash that uses case insensitive keys.
 */
uint64_t fnv1a_hash_str_casecmp(const void *key, uint64_t seed)
{
    return __fnv1a_hash(key, seed, true);
}

#endif

/**
 * str_eq:
 *      Case sensitive string comparison function.
 */
bool str_eq(const void *a, const void *b)
{
    return (strcmp(a, b) == 0) ? true : false;
}

/**
 * str_caseeq:
 *      Case insensitive string comparison function.
 */
bool str_caseeq(const void *a, const void *b)
{
    return (strcasecmp(a, b) == 0) ? true : false;
}

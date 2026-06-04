/* ht_fnv1a.c - FNV1A hash algorithm implementation.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2024
 */

#include "ht.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>

/// Return a hash key using the FNV1A algorithm (64-bit).
static ht_hash_t __fnv1a_hash(const void *key, ht_hash_t seed,
                              bool ignore_case) {
    ht_hash_t h = seed;

    for (const unsigned char *p = (const unsigned char *)key; *p; p++) {
        ht_hash_t c = (ht_hash_t)*p;
        if (ignore_case) {
            c = (ht_hash_t)tolower((int)c);
        }
        h ^= c;
        h *= FNV1A_PRIME;
    }

    return h;
}

/// Wrapper around __fnv1a_hash that uses case sensitive keys.
ht_hash_t fnv1a_hash_str(const void *key, ht_hash_t seed) {
    return __fnv1a_hash(key, seed, false);
}

/// Wrapper around __fnv1a_hash that uses case insensitive keys.
ht_hash_t fnv1a_hash_str_casecmp(const void *key, ht_hash_t seed) {
    return __fnv1a_hash(key, seed, true);
}

/// Case sensitive string comparison function.
bool str_eq(const void *a, const void *b) { return strcmp(a, b) == 0; }

/// Case insensitive string comparison function.
bool str_caseeq(const void *a, const void *b) { return strcasecmp(a, b) == 0; }

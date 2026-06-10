/**
 * @file ht_fnv1a.c
 * @brief FNV1A hash algorithm implementation.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 */

#include "ht.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/// Return a hash key using the FNV1A algorithm (64-bit).
static ht_hash_t __fnv1a_hash(const void *key, size_t len, bool ignore_case) {
    ht_hash_t h = FNV1A_OFFSET;
    const unsigned char *p = (const unsigned char *)key;

    for (size_t i = 0; i < len; i++) {
        ht_hash_t c = (ht_hash_t)p[i];
        if (ignore_case) {
            c = (ht_hash_t)tolower((int)c);
        }
        h ^= c;
        h *= FNV1A_PRIME;
    }

    return h;
}

/// Wrapper around __fnv1a_hash that uses case sensitive keys. The hashkey
/// argument is unused: FNV-1a is an unkeyed hash.
ht_hash_t ht_hash_fnv1a(const void *key, size_t len, const void *hashkey) {
    (void)hashkey;
    return __fnv1a_hash(key, len, false);
}

/// Wrapper around __fnv1a_hash that uses case insensitive keys. The hashkey
/// argument is unused: FNV-1a is an unkeyed hash.
ht_hash_t ht_hash_fnv1a_casecmp(const void *key, size_t len,
                                const void *hashkey) {
    (void)hashkey;
    return __fnv1a_hash(key, len, true);
}

/// Case sensitive string comparison function.
bool str_eq(const void *a, const void *b) { return strcmp(a, b) == 0; }

/// Case insensitive string comparison function.
bool str_caseeq(const void *a, const void *b) { return strcasecmp(a, b) == 0; }

/// String key length (excludes the terminating NUL).
size_t str_len(const void *key) { return strlen(key); }

/// Duplicate a NUL-terminated string key. The user_data argument is unused.
void *str_copy(const void *key, void *user_data) {
    (void)user_data;
    return strdup(key);
}

/// Free a string key previously produced by str_copy. The user_data argument
/// is unused.
void str_free(const void *key, void *user_data) {
    (void)user_data;
    free((void *)key);
}

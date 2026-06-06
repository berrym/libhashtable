/* ht_int.c - Integer-key hash via the splitmix64 (Stafford Mix13) finalizer.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2024
 */

#include "ht.h"

#include <string.h>

/// Hash an integer-valued key with the splitmix64 finalizer (Stafford's
/// variant 13). The first min(len, sizeof(uint64_t)) bytes of key are read as
/// the integer value; the hashkey argument is unused, as this is an unkeyed
/// hash. The finalizer is a bijection on 64-bit values, so distinct keys of a
/// given width never collide.
ht_hash_t ht_hash_int(const void *key, size_t len, const void *hashkey) {
    (void)hashkey;

    uint64_t z = 0;
    const size_t n = len < sizeof(z) ? len : sizeof(z);
    memcpy(&z, key, n);

    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

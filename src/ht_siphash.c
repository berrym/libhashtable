/* ht_siphash.c - Keyed SipHash for adversarial keys.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2024
 */

#include "ht.h"

#include "siphash.h"

/// SipHash-1-3 keyed hash. hashkey must point to a 16-byte key. Suited to
/// untrusted keys: when keyed with random material it resists the collision
/// engineering that defeats unkeyed hashes. This is the faster default.
ht_hash_t ht_hash_siphash(const void *key, size_t len, const void *hashkey) {
    return ht_siphash(key, len, hashkey, 1, 3);
}

/// SipHash-2-4 keyed hash, the conservative variant with a larger round count.
/// hashkey must point to a 16-byte key.
ht_hash_t ht_hash_siphash24(const void *key, size_t len, const void *hashkey) {
    return ht_siphash(key, len, hashkey, 2, 4);
}

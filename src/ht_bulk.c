/**
 * @file ht_bulk.c
 * @brief Bulk hash for long string and binary keys (wyhash).
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 */

#include "ht.h"

#include "vendor/wyhash.h"

/// Hash a key of arbitrary length with wyhash, using a fixed zero seed and the
/// default secret. Suited to long strings and binary blobs. The hashkey
/// argument is unused: this is an unkeyed hash for speed, not a
/// flooding-resistant PRF. Use the SipHash hash for adversarial input.
ht_hash_t ht_hash_bulk(const void *key, size_t len, const void *hashkey) {
    (void)hashkey;
    return wyhash(key, len, 0, _wyp);
}

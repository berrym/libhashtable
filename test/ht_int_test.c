/**
 * @file ht_int_test.c
 * @brief Test program for ht_hash_int.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 */

#include "ht.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    uint64_t a = 1, b = 2, c = 0x100000000ULL;
    ht_hash_t ha = ht_hash_int(&a, sizeof(a), NULL);
    ht_hash_t hb = ht_hash_int(&b, sizeof(b), NULL);
    ht_hash_t hc = ht_hash_int(&c, sizeof(c), NULL);

    /// The finalizer is a bijection, so distinct keys never collide.
    if (ha == hb || ha == hc || hb == hc) {
        fprintf(stderr, "ht_hash_int collided on distinct keys\n");
        exit(EXIT_FAILURE);
    }

    /// Hashing is deterministic.
    if (ht_hash_int(&a, sizeof(a), NULL) != ha) {
        fprintf(stderr, "ht_hash_int is not deterministic\n");
        exit(EXIT_FAILURE);
    }

    /// A single-bit input change changes the hash.
    uint64_t x = 0x0123456789abcdefULL;
    uint64_t y = x ^ 1u;
    if (ht_hash_int(&x, sizeof(x), NULL) == ht_hash_int(&y, sizeof(y), NULL)) {
        fprintf(stderr, "ht_hash_int did not respond to a 1-bit change\n");
        exit(EXIT_FAILURE);
    }

    /// Distinct 4-byte keys produce distinct hashes (a partial-width read).
    uint32_t s1 = 0xdeadbeefU, s2 = 0xdeadbef0U;
    if (ht_hash_int(&s1, sizeof(s1), NULL) ==
        ht_hash_int(&s2, sizeof(s2), NULL)) {
        fprintf(stderr, "ht_hash_int collided on distinct 4-byte keys\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

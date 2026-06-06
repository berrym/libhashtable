/**
 * @file ht_stats_test.c
 * @brief Test program for per-table structural statistics
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 */

#include "ht.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const ht_callbacks_t cb = {str_copy, str_free, str_copy, str_free};
    ht_t *ht = ht_create(&(ht_options_t){.hash = ht_hash_fnv1a,
                                         .keyeq = str_eq,
                                         .keylen = str_len,
                                         .callbacks = cb});
    if (!ht) {
        fprintf(stderr, "ht_create failed\n");
        exit(EXIT_FAILURE);
    }

    ht_stats_t s;

    /// A fresh table has its default capacity, no entries, and no grows.
    ht_stats(ht, &s);
    if (s.size != 0 || s.occupied_buckets != 0 || s.max_chain != 0 ||
        s.grow_count != 0 || s.load_factor != 0.0) {
        fprintf(stderr, "empty-table stats are not all zero\n");
        exit(EXIT_FAILURE);
    }
    if (s.capacity == 0) {
        fprintf(stderr, "empty table reported zero capacity\n");
        exit(EXIT_FAILURE);
    }
    const size_t initial_capacity = s.capacity;

    /// Insert enough distinct keys to force at least one grow-and-rehash.
    enum { N = 200 };
    for (size_t i = 0; i < N; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key-%zu", i);
        ht_insert(ht, key, "v");
    }

    ht_stats(ht, &s);

    /// size reflects every inserted entry and agrees with ht_size.
    if (s.size != N || s.size != ht_size(ht)) {
        fprintf(stderr, "size %zu, expected %d\n", s.size, N);
        exit(EXIT_FAILURE);
    }

    /// The table grew, so capacity increased and grow_count advanced.
    if (s.grow_count == 0 || s.capacity <= initial_capacity) {
        fprintf(stderr, "expected a grow: grow_count=%zu capacity=%zu\n",
                s.grow_count, s.capacity);
        exit(EXIT_FAILURE);
    }

    /// Structural invariants: every entry occupies a slot, chains are bounded
    /// by the entry count, and the table keeps its load under the grow
    /// threshold.
    if (s.occupied_buckets == 0 || s.occupied_buckets > s.size) {
        fprintf(stderr, "occupied_buckets %zu out of range for size %zu\n",
                s.occupied_buckets, s.size);
        exit(EXIT_FAILURE);
    }
    if (s.max_chain < 1 || s.max_chain > s.size) {
        fprintf(stderr, "max_chain %zu out of range for size %zu\n",
                s.max_chain, s.size);
        exit(EXIT_FAILURE);
    }
    if (s.load_factor != (double)s.size / (double)s.capacity) {
        fprintf(stderr, "load_factor does not match size/capacity\n");
        exit(EXIT_FAILURE);
    }
    if (s.load_factor >= 0.75) {
        fprintf(stderr, "load_factor %.3f at or above the grow threshold\n",
                s.load_factor);
        exit(EXIT_FAILURE);
    }

    /// Clearing empties the table but preserves the grown capacity and the
    /// cumulative grow count.
    const size_t grown_capacity = s.capacity;
    const size_t grows = s.grow_count;
    ht_clear(ht);
    ht_stats(ht, &s);
    if (s.size != 0 || s.occupied_buckets != 0 || s.max_chain != 0) {
        fprintf(stderr, "stats not empty after ht_clear\n");
        exit(EXIT_FAILURE);
    }
    if (s.capacity != grown_capacity || s.grow_count != grows) {
        fprintf(stderr, "ht_clear changed capacity or grow_count\n");
        exit(EXIT_FAILURE);
    }

    ht_destroy(ht);

    /// A NULL table fills the snapshot with zeros; a NULL snapshot is ignored.
    ht_stats_t z;
    ht_stats(NULL, &z);
    if (z.size != 0 || z.capacity != 0 || z.grow_count != 0 ||
        z.load_factor != 0.0) {
        fprintf(stderr, "NULL-table stats are not zeroed\n");
        exit(EXIT_FAILURE);
    }
    ht_stats(NULL, NULL);

    return 0;
}

/**
 * @file ht_ops_test.c
 * @brief Test program for the compound table operations
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
#include <string.h>

static void count_visit(const void *key, const void *val, void *user_data) {
    (void)key;
    (void)val;
    (*(size_t *)user_data)++;
}

int main(void) {
    const ht_callbacks_t cb = {
        (void *(*)(const void *))strdup, (void (*)(const void *))free,
        (void *(*)(const void *))strdup, (void (*)(const void *))free};
    ht_t *ht = ht_create(&(ht_options_t){.hash = ht_hash_fnv1a,
                                         .keyeq = str_eq,
                                         .keylen = str_len,
                                         .callbacks = cb});
    if (!ht) {
        fprintf(stderr, "ht_create failed\n");
        exit(EXIT_FAILURE);
    }

    if (ht_size(ht) != 0) {
        fprintf(stderr, "empty size != 0\n");
        exit(EXIT_FAILURE);
    }

    ht_insert(ht, "a", "1");
    ht_insert(ht, "b", "2");
    if (ht_size(ht) != 2) {
        fprintf(stderr, "size != 2\n");
        exit(EXIT_FAILURE);
    }

    if (!ht_contains(ht, "a") || ht_contains(ht, "z")) {
        fprintf(stderr, "ht_contains wrong\n");
        exit(EXIT_FAILURE);
    }

    /// get_or_insert returns the existing value and does not overwrite.
    const char *v = ht_get_or_insert(ht, "a", "OVERWRITE");
    if (!v || strcmp(v, "1") != 0 || strcmp(ht_get(ht, "a"), "1") != 0) {
        fprintf(stderr, "ht_get_or_insert overwrote an existing key\n");
        exit(EXIT_FAILURE);
    }

    /// get_or_insert inserts an absent key and returns the new value.
    v = ht_get_or_insert(ht, "c", "3");
    if (!v || strcmp(v, "3") != 0 || ht_size(ht) != 3) {
        fprintf(stderr, "ht_get_or_insert did not insert\n");
        exit(EXIT_FAILURE);
    }

    /// upsert reports newly-added vs replaced.
    if (!ht_upsert(ht, "d", "4")) {
        fprintf(stderr, "ht_upsert of a new key should return true\n");
        exit(EXIT_FAILURE);
    }
    if (ht_upsert(ht, "d", "44") || strcmp(ht_get(ht, "d"), "44") != 0) {
        fprintf(stderr, "ht_upsert of an existing key should replace and "
                        "return false\n");
        exit(EXIT_FAILURE);
    }

    /// foreach visits every entry exactly once.
    size_t n = 0;
    ht_foreach(ht, count_visit, &n);
    if (n != ht_size(ht)) {
        fprintf(stderr, "ht_foreach visited %zu of %zu entries\n", n,
                ht_size(ht));
        exit(EXIT_FAILURE);
    }

    /// clear empties the table but leaves it reusable.
    ht_clear(ht);
    if (ht_size(ht) != 0 || ht_contains(ht, "a")) {
        fprintf(stderr, "ht_clear did not empty the table\n");
        exit(EXIT_FAILURE);
    }
    ht_insert(ht, "x", "9");
    if (ht_size(ht) != 1 || strcmp(ht_get(ht, "x"), "9") != 0) {
        fprintf(stderr, "table not reusable after ht_clear\n");
        exit(EXIT_FAILURE);
    }

    ht_destroy(ht);
    return 0;
}

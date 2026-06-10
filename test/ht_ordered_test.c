/**
 * @file ht_ordered_test.c
 * @brief Test program for insertion-order iteration
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

/// A degenerate hash that forces every key into one bucket, so the collision
/// chain and the head-promotion removal path are exercised deterministically.
static ht_hash_t all_collide(const void *key, size_t len, const void *hashkey) {
    (void)key;
    (void)len;
    (void)hashkey;
    return 0;
}

/// Confirm the cursor yields exactly the keys in want, in order, with matching
/// values, then ends.
static void expect_order(ht_t *ht, const char *const *want,
                         const char *const *want_val, size_t n) {
    ht_enum_t *he = ht_enum_create(ht);
    if (!he) {
        fprintf(stderr, "ht_enum_create failed\n");
        exit(EXIT_FAILURE);
    }

    const char *k = NULL;
    const char *v = NULL;
    size_t i = 0;
    while (ht_enum_next(he, (const void **)&k, (const void **)&v)) {
        if (i >= n) {
            fprintf(stderr, "cursor yielded more than %zu entries\n", n);
            exit(EXIT_FAILURE);
        }
        if (strcmp(k, want[i]) != 0) {
            fprintf(stderr, "position %zu: key %s, expected %s\n", i, k,
                    want[i]);
            exit(EXIT_FAILURE);
        }
        if (strcmp(v, want_val[i]) != 0) {
            fprintf(stderr, "position %zu: value %s, expected %s\n", i, v,
                    want_val[i]);
            exit(EXIT_FAILURE);
        }
        i++;
    }
    if (i != n) {
        fprintf(stderr, "cursor yielded %zu entries, expected %zu\n", i, n);
        exit(EXIT_FAILURE);
    }

    ht_enum_destroy(he);
}

/// Insertion order is preserved across the grow-and-rehash boundary, and a
/// replaced key keeps its position while reporting its new value.
static void test_order_through_rehash(void) {
    const ht_callbacks_t cb = {str_copy, str_free, str_copy, str_free};
    ht_t *ht = ht_create(&(ht_options_t){.hash = ht_hash_fnv1a,
                                         .keyeq = str_eq,
                                         .keylen = str_len,
                                         .callbacks = cb,
                                         .insertion_ordered = true});
    if (!ht) {
        fprintf(stderr, "ht_create failed\n");
        exit(EXIT_FAILURE);
    }

    enum { N = 200 };
    char keys[N][16];
    char vals[N][16];
    const char *kp[N];
    const char *vp[N];
    for (size_t i = 0; i < N; i++) {
        snprintf(keys[i], sizeof(keys[i]), "k%zu", i);
        snprintf(vals[i], sizeof(vals[i]), "v%zu", i);
        ht_insert(ht, keys[i], vals[i]);
        kp[i] = keys[i];
        vp[i] = vals[i];
    }

    /// Every key enumerates in insertion order despite several rehashes.
    expect_order(ht, kp, vp, N);

    /// Replacing a value leaves the key where it was first inserted.
    ht_insert(ht, "k100", "REPLACED");
    vp[100] = "REPLACED";
    expect_order(ht, kp, vp, N);

    /// Remove the first, last, and a middle key; the survivors keep their
    /// order.
    ht_remove(ht, "k0");
    ht_remove(ht, "k199");
    ht_remove(ht, "k100");

    const char *ek[N];
    const char *ev[N];
    size_t m = 0;
    for (size_t i = 0; i < N; i++) {
        if (i == 0 || i == 100 || i == 199) {
            continue;
        }
        ek[m] = kp[i];
        ev[m] = vp[i];
        m++;
    }
    expect_order(ht, ek, ev, m);
    if (ht_size(ht) != m) {
        fprintf(stderr, "size %zu after removals, expected %zu\n", ht_size(ht),
                m);
        exit(EXIT_FAILURE);
    }

    ht_destroy(ht);
}

/// The head-promotion removal path keeps order intact and re-points the
/// promoted entry's order node at its freshly copied key.
static void test_collision_chain(void) {
    const ht_callbacks_t cb = {str_copy, str_free, str_copy, str_free};
    ht_t *ht = ht_create(&(ht_options_t){.hash = all_collide,
                                         .keyeq = str_eq,
                                         .keylen = str_len,
                                         .callbacks = cb,
                                         .insertion_ordered = true});
    if (!ht) {
        fprintf(stderr, "ht_create failed\n");
        exit(EXIT_FAILURE);
    }

    ht_insert(ht, "a", "1");
    ht_insert(ht, "b", "2");
    ht_insert(ht, "c", "3");
    ht_insert(ht, "d", "4");

    expect_order(ht, (const char *[]){"a", "b", "c", "d"},
                 (const char *[]){"1", "2", "3", "4"}, 4);

    /// Removing the chain head promotes "b" into the head slot, re-copying its
    /// key; order and value lookups must remain correct.
    ht_remove(ht, "a");
    if (strcmp(ht_get(ht, "b"), "2") != 0 ||
        strcmp(ht_get(ht, "d"), "4") != 0) {
        fprintf(stderr, "lookup wrong after head removal\n");
        exit(EXIT_FAILURE);
    }
    expect_order(ht, (const char *[]){"b", "c", "d"},
                 (const char *[]){"2", "3", "4"}, 3);

    /// Removing an interior chain node leaves the rest in order.
    ht_remove(ht, "c");
    expect_order(ht, (const char *[]){"b", "d"}, (const char *[]){"2", "4"}, 2);

    /// clear empties the order list but leaves the table reusable and ordered.
    ht_clear(ht);
    expect_order(ht, NULL, NULL, 0);
    ht_insert(ht, "x", "9");
    ht_insert(ht, "y", "8");
    expect_order(ht, (const char *[]){"x", "y"}, (const char *[]){"9", "8"}, 2);

    ht_destroy(ht);
}

/// A typed wrapper enumerates in insertion order when built with the
/// insertion_ordered option.
static void test_typed_wrapper_order(void) {
    ht_strstr_t *ht =
        ht_strstr_create(&(ht_str_options_t){.insertion_ordered = true});
    if (!ht) {
        fprintf(stderr, "ht_strstr_create failed\n");
        exit(EXIT_FAILURE);
    }

    ht_strstr_insert(ht, "one", "1");
    ht_strstr_insert(ht, "two", "2");
    ht_strstr_insert(ht, "three", "3");
    ht_strstr_remove(ht, "two");
    ht_strstr_insert(ht, "four", "4");

    const char *const want[] = {"one", "three", "four"};
    const char *const want_val[] = {"1", "3", "4"};

    ht_enum_t *he = ht_strstr_enum_create(ht);
    if (!he) {
        fprintf(stderr, "ht_strstr_enum_create failed\n");
        exit(EXIT_FAILURE);
    }

    const char *k = NULL;
    const char *v = NULL;
    size_t i = 0;
    while (ht_strstr_enum_next(he, &k, &v)) {
        if (i >= 3 || strcmp(k, want[i]) != 0 || strcmp(v, want_val[i]) != 0) {
            fprintf(stderr, "wrapper order wrong at %zu\n", i);
            exit(EXIT_FAILURE);
        }
        i++;
    }
    if (i != 3) {
        fprintf(stderr, "wrapper yielded %zu entries, expected 3\n", i);
        exit(EXIT_FAILURE);
    }

    ht_strstr_enum_destroy(he);
    ht_strstr_destroy(ht);
}

int main(void) {
    test_order_through_rehash();
    test_collision_chain();
    test_typed_wrapper_order();
    return 0;
}

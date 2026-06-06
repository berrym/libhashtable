/**
 * @file ht_strset_test.c
 * @brief Test program for the string set
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
    ht_strset_t *s = ht_strset_create(NULL);
    if (!s) {
        fprintf(stderr, "ht_strset_create failed\n");
        exit(EXIT_FAILURE);
    }

    /// add reports newly-added vs already-present.
    if (!ht_strset_add(s, "alpha")) {
        fprintf(stderr, "adding a new member should return true\n");
        exit(EXIT_FAILURE);
    }
    if (ht_strset_add(s, "alpha")) {
        fprintf(stderr, "adding a duplicate should return false\n");
        exit(EXIT_FAILURE);
    }
    ht_strset_add(s, "beta");

    /// contains reflects membership.
    if (!ht_strset_contains(s, "alpha") || !ht_strset_contains(s, "beta")) {
        fprintf(stderr, "contains missed a present member\n");
        exit(EXIT_FAILURE);
    }
    if (ht_strset_contains(s, "gamma")) {
        fprintf(stderr, "contains reported an absent member\n");
        exit(EXIT_FAILURE);
    }

    /// remove drops a member.
    ht_strset_remove(s, "alpha");
    if (ht_strset_contains(s, "alpha")) {
        fprintf(stderr, "remove failed\n");
        exit(EXIT_FAILURE);
    }

    /// enumeration yields the remaining members.
    ht_enum_t *he = ht_strset_enum_create(s);
    if (!he) {
        ht_strset_destroy(s);
        exit(EXIT_FAILURE);
    }
    const char *k = NULL;
    size_t count = 0;
    while (ht_strset_enum_next(he, &k)) {
        count++;
    }
    ht_strset_enum_destroy(he);
    if (count != 1) {
        fprintf(stderr, "enumeration visited %zu members, expected 1\n", count);
        exit(EXIT_FAILURE);
    }

    ht_strset_destroy(s);
    return 0;
}

/**
 * @file ht_u64ptr_test.c
 * @brief Test program for the uint64->pointer hash table
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
    ht_u64ptr_t *ht = ht_u64ptr_create(NULL);
    if (!ht) {
        fprintf(stderr, "ht_u64ptr_create failed\n");
        exit(EXIT_FAILURE);
    }

    int objects[3] = {10, 20, 30};

    /// The table stores caller-owned pointers keyed by uint64.
    ht_u64ptr_insert(ht, 1, &objects[0]);
    ht_u64ptr_insert(ht, 2, &objects[1]);
    ht_u64ptr_insert(ht, 0x100000000ULL, &objects[2]);

    if (ht_u64ptr_get(ht, 1) != &objects[0] ||
        ht_u64ptr_get(ht, 2) != &objects[1] ||
        ht_u64ptr_get(ht, 0x100000000ULL) != &objects[2]) {
        fprintf(stderr, "lookup returned the wrong pointer\n");
        exit(EXIT_FAILURE);
    }

    /// An absent key returns NULL.
    if (ht_u64ptr_get(ht, 99) != NULL) {
        fprintf(stderr, "absent key did not return NULL\n");
        exit(EXIT_FAILURE);
    }

    /// Keys differing only in their high 32 bits are distinct.
    if (ht_u64ptr_get(ht, 0x100000000ULL) == ht_u64ptr_get(ht, 0)) {
        fprintf(stderr, "high-bit key collision\n");
        exit(EXIT_FAILURE);
    }

    /// Replacing a key updates the stored pointer.
    ht_u64ptr_insert(ht, 1, &objects[2]);
    if (ht_u64ptr_get(ht, 1) != &objects[2]) {
        fprintf(stderr, "replacement did not update the value\n");
        exit(EXIT_FAILURE);
    }

    /// Removal drops the entry.
    ht_u64ptr_remove(ht, 1);
    if (ht_u64ptr_get(ht, 1) != NULL) {
        fprintf(stderr, "removal failed\n");
        exit(EXIT_FAILURE);
    }

    /// Enumeration yields each key with its pointer.
    ht_enum_t *he = ht_u64ptr_enum_create(ht);
    if (!he) {
        ht_u64ptr_destroy(ht);
        exit(EXIT_FAILURE);
    }
    uint64_t ek = 0;
    void *ev = NULL;
    size_t count = 0;
    while (ht_u64ptr_enum_next(he, &ek, &ev)) {
        count++;
    }
    ht_u64ptr_enum_destroy(he);
    if (count != 2) {
        fprintf(stderr, "enumeration visited %zu entries, expected 2\n", count);
        exit(EXIT_FAILURE);
    }

    ht_u64ptr_destroy(ht);
    return 0;
}

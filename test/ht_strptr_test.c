/**
 * @file ht_strptr_test.c
 * @brief Test program for the string->pointer hash table
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
    ht_strptr_t *ht = ht_strptr_create(NULL);
    if (!ht) {
        fprintf(stderr, "ht_strptr_create failed\n");
        exit(EXIT_FAILURE);
    }

    int objects[3] = {10, 20, 30};

    /// The exact pointer is stored and returned, with no string round-trip.
    ht_strptr_insert(ht, "alpha", &objects[0]);
    ht_strptr_insert(ht, "beta", &objects[1]);
    if (ht_strptr_get(ht, "alpha") != &objects[0] ||
        ht_strptr_get(ht, "beta") != &objects[1]) {
        fprintf(stderr, "lookup returned the wrong pointer\n");
        exit(EXIT_FAILURE);
    }

    /// An absent key returns NULL.
    if (ht_strptr_get(ht, "absent") != NULL) {
        fprintf(stderr, "absent key did not return NULL\n");
        exit(EXIT_FAILURE);
    }

    /// Replacing a key updates the stored pointer.
    ht_strptr_insert(ht, "alpha", &objects[2]);
    if (ht_strptr_get(ht, "alpha") != &objects[2]) {
        fprintf(stderr, "replacement did not update the value\n");
        exit(EXIT_FAILURE);
    }

    /// Removal drops the entry.
    ht_strptr_remove(ht, "beta");
    if (ht_strptr_get(ht, "beta") != NULL) {
        fprintf(stderr, "removal failed\n");
        exit(EXIT_FAILURE);
    }

    /// Enumeration yields each key with its pointer.
    ht_enum_t *he = ht_strptr_enum_create(ht);
    if (!he) {
        ht_strptr_destroy(ht);
        exit(EXIT_FAILURE);
    }
    const char *ek = NULL;
    void *ev = NULL;
    size_t count = 0;
    while (ht_strptr_enum_next(he, &ek, &ev)) {
        count++;
    }
    ht_strptr_enum_destroy(he);
    if (count != 1) {
        fprintf(stderr, "enumeration visited %zu entries, expected 1\n", count);
        exit(EXIT_FAILURE);
    }

    ht_strptr_destroy(ht);
    return 0;
}

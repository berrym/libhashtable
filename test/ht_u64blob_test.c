/**
 * @file ht_u64blob_test.c
 * @brief Test program for the uint64->blob hash table
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
#include <string.h>

int main(void) {
    ht_u64blob_t *ht = ht_u64blob_create(NULL);
    if (!ht) {
        fprintf(stderr, "ht_u64blob_create failed\n");
        exit(EXIT_FAILURE);
    }

    /// A binary value with embedded NUL bytes round-trips under an integer key.
    const unsigned char blob[] = {0x00, 0xff, 0x00, 0x42, 0x00};
    ht_u64blob_insert(ht, 7, blob, sizeof(blob));

    size_t n = 0;
    const void *got = ht_u64blob_get(ht, 7, &n);
    if (!got || n != sizeof(blob) || memcmp(got, blob, sizeof(blob)) != 0) {
        fprintf(stderr, "blob round-trip under integer key failed\n");
        exit(EXIT_FAILURE);
    }

    /// A large integer key is distinct from a small one.
    const unsigned char other[] = {0xaa};
    ht_u64blob_insert(ht, 0x100000007ULL, other, sizeof(other));
    got = ht_u64blob_get(ht, 7, &n);
    if (!got || n != sizeof(blob)) {
        fprintf(stderr, "high-bit key collided with a low key\n");
        exit(EXIT_FAILURE);
    }

    /// An absent key returns NULL and zeroes the size.
    n = 123;
    if (ht_u64blob_get(ht, 99, &n) != NULL || n != 0) {
        fprintf(stderr, "absent-key lookup wrong\n");
        exit(EXIT_FAILURE);
    }

    /// Replacement updates the stored blob.
    const unsigned char blob2[] = {0x01, 0x02};
    ht_u64blob_insert(ht, 7, blob2, sizeof(blob2));
    got = ht_u64blob_get(ht, 7, &n);
    if (!got || n != sizeof(blob2) || memcmp(got, blob2, sizeof(blob2)) != 0) {
        fprintf(stderr, "blob replacement failed\n");
        exit(EXIT_FAILURE);
    }

    /// Enumeration yields each key with its blob and length.
    ht_enum_t *he = ht_u64blob_enum_create(ht);
    if (!he) {
        ht_u64blob_destroy(ht);
        exit(EXIT_FAILURE);
    }
    uint64_t ek = 0;
    const void *ev = NULL;
    size_t es = 0;
    size_t count = 0;
    while (ht_u64blob_enum_next(he, &ek, &ev, &es)) {
        count++;
    }
    ht_u64blob_enum_destroy(he);
    if (count != 2) {
        fprintf(stderr, "enumeration visited %zu entries, expected 2\n", count);
        exit(EXIT_FAILURE);
    }

    ht_u64blob_destroy(ht);
    return 0;
}

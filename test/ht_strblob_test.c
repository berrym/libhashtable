/* ht_strblob_test.c - Test program for the string->blob hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2024
 */

#include "ht.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    ht_strblob_t *ht = ht_strblob_create(NULL);
    if (!ht) {
        fprintf(stderr, "ht_strblob_create failed\n");
        exit(EXIT_FAILURE);
    }

    /* A value with embedded NUL bytes round-trips in full. This is the reason
     * ht_strblob exists: ht_strstr would truncate at the first NUL. */
    const unsigned char blob[] = {0xde, 0x00, 0xad, 0x00, 0xbe, 0xef};
    ht_strblob_insert(ht, "k", blob, sizeof(blob));

    size_t n = 0;
    const void *got = ht_strblob_get(ht, "k", &n);
    if (!got || n != sizeof(blob) || memcmp(got, blob, sizeof(blob)) != 0) {
        fprintf(stderr, "blob round-trip with embedded NUL failed\n");
        exit(EXIT_FAILURE);
    }

    /* A zero-length blob is valid. */
    ht_strblob_insert(ht, "empty", NULL, 0);
    got = ht_strblob_get(ht, "empty", &n);
    if (n != 0) {
        fprintf(stderr, "zero-length blob reported size %zu\n", n);
        exit(EXIT_FAILURE);
    }

    /* An absent key returns NULL and zeroes the size. */
    n = 123;
    if (ht_strblob_get(ht, "absent", &n) != NULL || n != 0) {
        fprintf(stderr, "absent-key lookup wrong\n");
        exit(EXIT_FAILURE);
    }

    /* Replacing a key frees the old blob and stores the new one. */
    const unsigned char blob2[] = {0x01, 0x02, 0x03};
    ht_strblob_insert(ht, "k", blob2, sizeof(blob2));
    got = ht_strblob_get(ht, "k", &n);
    if (!got || n != sizeof(blob2) || memcmp(got, blob2, sizeof(blob2)) != 0) {
        fprintf(stderr, "blob replacement failed\n");
        exit(EXIT_FAILURE);
    }

    /* Enumeration yields each key with its blob and length. */
    ht_enum_t *he = ht_strblob_enum_create(ht);
    if (!he) {
        ht_strblob_destroy(ht);
        exit(EXIT_FAILURE);
    }
    const char *ek = NULL;
    const void *ev = NULL;
    size_t es = 0;
    size_t count = 0;
    while (ht_strblob_enum_next(he, &ek, &ev, &es)) {
        count++;
    }
    ht_strblob_enum_destroy(he);
    if (count != 2) {
        fprintf(stderr, "enumeration visited %zu entries, expected 2\n", count);
        exit(EXIT_FAILURE);
    }

    ht_strblob_destroy(ht);
    return 0;
}

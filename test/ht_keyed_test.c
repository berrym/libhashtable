/* ht_keyed_test.c - Test program for keyed-table creation at the base layer.
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
    const ht_callbacks_t cb = {
        (void *(*)(const void *))strdup, (void (*)(const void *))free,
        (void *(*)(const void *))strdup, (void (*)(const void *))free};

    /* A SipHash-keyed table created at the base layer works end to end. The
     * table must pass a non-NULL key to SipHash; if the wiring passed NULL this
     * would dereference a null pointer. */
    ht_t *ht = ht_create(&(ht_options_t){.hash = ht_hash_siphash,
                                         .keyeq = str_eq,
                                         .keylen = str_len,
                                         .callbacks = cb,
                                         .key_mode = HT_KEY_RANDOM});
    if (!ht) {
        fprintf(stderr, "ht_create with SipHash + HT_KEY_RANDOM failed\n");
        exit(EXIT_FAILURE);
    }

    ht_insert(ht, "alpha", "one");
    ht_insert(ht, "beta", "two");
    ht_insert(ht, "gamma", "three");

    const char *v = ht_get(ht, "beta");
    if (!v || strcmp(v, "two") != 0) {
        fprintf(stderr, "keyed table lookup failed\n");
        exit(EXIT_FAILURE);
    }

    ht_remove(ht, "beta");
    if (ht_get(ht, "beta") != NULL) {
        fprintf(stderr, "keyed table remove failed\n");
        exit(EXIT_FAILURE);
    }

    ht_destroy(ht);

    /* An unkeyed table (FNV-1a, HT_KEY_NONE) also works: the hash receives a
     * NULL hashkey and ignores it. */
    ht_t *ht2 = ht_create(&(ht_options_t){.hash = ht_hash_fnv1a,
                                          .keyeq = str_eq,
                                          .keylen = str_len,
                                          .callbacks = cb});
    if (!ht2) {
        fprintf(stderr, "ht_create unkeyed failed\n");
        exit(EXIT_FAILURE);
    }

    ht_insert(ht2, "x", "1");
    v = ht_get(ht2, "x");
    if (!v || strcmp(v, "1") != 0) {
        fprintf(stderr, "unkeyed table lookup failed\n");
        exit(EXIT_FAILURE);
    }

    ht_destroy(ht2);

    return 0;
}

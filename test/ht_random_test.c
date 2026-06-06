/* ht_random_test.c - Test program for ht_random_bytes.
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
    unsigned char a[32] = {0};
    unsigned char b[32] = {0};

    /* A request for entropy succeeds. */
    if (ht_random_bytes(a, sizeof(a)) != 0) {
        fprintf(stderr, "ht_random_bytes failed\n");
        exit(EXIT_FAILURE);
    }

    /* The buffer is not a degenerate constant fill. For 32 random bytes the
     * probability of this check failing by chance is about 2^-248. */
    bool varied = false;
    for (size_t i = 1; i < sizeof(a); i++) {
        if (a[i] != a[0]) {
            varied = true;
            break;
        }
    }
    if (!varied) {
        fprintf(stderr, "ht_random_bytes produced a constant buffer\n");
        exit(EXIT_FAILURE);
    }

    /* Two independent fills differ. The chance of a false failure is 2^-256. */
    if (ht_random_bytes(b, sizeof(b)) != 0) {
        fprintf(stderr, "ht_random_bytes failed\n");
        exit(EXIT_FAILURE);
    }
    if (memcmp(a, b, sizeof(a)) == 0) {
        fprintf(stderr, "two ht_random_bytes calls returned identical data\n");
        exit(EXIT_FAILURE);
    }

    /* A zero-length request is a successful no-op. */
    if (ht_random_bytes(a, 0) != 0) {
        fprintf(stderr, "ht_random_bytes(buf, 0) did not succeed\n");
        exit(EXIT_FAILURE);
    }

    /* A NULL buffer is rejected. */
    if (ht_random_bytes(NULL, 16) != -1) {
        fprintf(stderr, "ht_random_bytes(NULL, 16) did not fail\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

/* ht_bulk_test.c - Test program for ht_hash_bulk (wyhash).
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2024
 *
 * The expected values are reference vectors generated from the upstream
 * wyhash v4.3 (the __uint128_t multiply path), so they verify that the
 * vendored portable-multiply subset reproduces the upstream algorithm across
 * its short, medium, and long-key code paths. Inputs are byte i = i * 7 + 1.
 */

#include "ht.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    static const struct {
        size_t len;
        uint64_t expected;
    } vectors[] = {
        { 0, 0x93228a4de0eec5a2ULL},
        { 1, 0x39a10bde68f15a1bULL},
        { 3, 0xe9629f9bd9227e63ULL},
        { 4, 0xf61024db5a715a40ULL},
        { 8, 0xef68240886d19638ULL},
        {14, 0x419e65d2e78d5560ULL},
        {16, 0xa9aecb23fb1cb420ULL},
        {17, 0xdb4098d10618bc3cULL},
        {32, 0x8082521dfb78d6d4ULL},
        {48, 0xaee015b89490a9c7ULL},
        {80, 0x601b5b2dafef7258ULL},
    };

    for (size_t v = 0; v < sizeof(vectors) / sizeof(vectors[0]); v++) {
        size_t n = vectors[v].len;
        unsigned char buf[128];
        for (size_t i = 0; i < n; i++) {
            buf[i] = (unsigned char)(i * 7 + 1);
        }
        ht_hash_t h = ht_hash_bulk(buf, n, NULL);
        if (h != vectors[v].expected) {
            fprintf(stderr,
                    "wyhash vector mismatch at len=%zu: got %016llx, "
                    "want %016llx\n",
                    n, (unsigned long long)h,
                    (unsigned long long)vectors[v].expected);
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

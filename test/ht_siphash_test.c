/* ht_siphash_test.c - Test program for ht_hash_siphash and ht_hash_siphash24.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2024
 *
 * The vectors use the standard SipHash test key (bytes 0x00..0x0f) and inputs
 * (byte i = i). The SipHash-2-4 column is the published reference set (the
 * len-15 value 0xa129ca6149be45e5 is the canonical paper vector); the
 * SipHash-1-3 column is generated from the same authenticated reference.
 */

#include "ht.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    unsigned char key[16];
    for (size_t i = 0; i < sizeof(key); i++) {
        key[i] = (unsigned char)i;
    }

    static const struct {
        size_t len;
        uint64_t e13;
        uint64_t e24;
    } vectors[] = {
        { 0, 0xabac0158050fc4dcULL, 0x726fdb47dd0e0e31ULL},
        { 1, 0xc9f49bf37d57ca93ULL, 0x74f839c593dc67fdULL},
        { 7, 0xd3927d989bb11140ULL, 0xab0200f58b01d137ULL},
        { 8, 0x369095118d299a8eULL, 0x93f5f5799a932462ULL},
        {15, 0xd320d86d2a519956ULL, 0xa129ca6149be45e5ULL},
        {16, 0xcc4fdd1a7d908b66ULL, 0x3f2acc7f57c29bdbULL},
        {31, 0x2370dd1f8c21d1bcULL, 0x32d892fad841c342ULL},
        {32, 0x81157b6c16a7b60dULL, 0x7127512f72f27cceULL},
        {63, 0x9d199062b7bbb3a8ULL, 0x958a324ceb064572ULL},
        {64, 0xf17997ec4b4a6065ULL, 0xacd2c40b8502cad8ULL},
    };

    for (size_t v = 0; v < sizeof(vectors) / sizeof(vectors[0]); v++) {
        size_t n = vectors[v].len;
        unsigned char in[64];
        for (size_t i = 0; i < n; i++) {
            in[i] = (unsigned char)i;
        }
        if (ht_hash_siphash(in, n, key) != vectors[v].e13) {
            fprintf(stderr, "SipHash-1-3 vector mismatch at len=%zu\n", n);
            exit(EXIT_FAILURE);
        }
        if (ht_hash_siphash24(in, n, key) != vectors[v].e24) {
            fprintf(stderr, "SipHash-2-4 vector mismatch at len=%zu\n", n);
            exit(EXIT_FAILURE);
        }
    }

    /* Keyed behavior: a fixed key is deterministic, and two distinct random
     * keys produce different hashes of the same input. */
    unsigned char k1[16], k2[16];
    const char *msg = "the quick brown fox";
    size_t mlen = strlen(msg);
    if (ht_random_bytes(k1, sizeof(k1)) != 0 ||
        ht_random_bytes(k2, sizeof(k2)) != 0) {
        fprintf(stderr, "ht_random_bytes failed\n");
        exit(EXIT_FAILURE);
    }
    if (ht_hash_siphash(msg, mlen, k1) != ht_hash_siphash(msg, mlen, k1)) {
        fprintf(stderr, "SipHash is not deterministic for a fixed key\n");
        exit(EXIT_FAILURE);
    }
    if (ht_hash_siphash(msg, mlen, k1) == ht_hash_siphash(msg, mlen, k2)) {
        fprintf(stderr, "SipHash ignored the key: k1 and k2 agree\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

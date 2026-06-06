/* siphash.h - SipHash reference (64-bit output), vendored.
 *
 * Source:    https://github.com/veorq/SipHash
 * Commit:    eee7d0d84dc7731df2359b243aa5e75d85f6eaef
 * Retrieved: 2026-06-05
 * Authors:   Jean-Philippe Aumasson, Daniel J. Bernstein.
 * License:   CC0 1.0 Universal public domain dedication
 *            (http://creativecommons.org/publicdomain/zero/1.0/). This
 *            software is distributed without any warranty.
 *
 * This is the portable subset used by libhashtable. The SipHash compression is
 * copied from the reference, with cROUNDS and dROUNDS made function parameters
 * so SipHash-1-3 and SipHash-2-4 share one implementation, and the 128-bit
 * (outlen == 16) output path omitted. The reference was verified against the
 * published vectors.h (all 64 SipHash-2-4 outputs match); at the standard key
 * the SipHash-2-4 output for a 15-byte input is 0xa129ca6149be45e5.
 */

#ifndef HT_SIPHASH_H
#define HT_SIPHASH_H

#include <stddef.h>
#include <stdint.h>

#define HT_SIP_ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define HT_SIP_U8TO64_LE(p)                                                    \
    (((uint64_t)((p)[0])) | ((uint64_t)((p)[1]) << 8) |                        \
     ((uint64_t)((p)[2]) << 16) | ((uint64_t)((p)[3]) << 24) |                 \
     ((uint64_t)((p)[4]) << 32) | ((uint64_t)((p)[5]) << 40) |                 \
     ((uint64_t)((p)[6]) << 48) | ((uint64_t)((p)[7]) << 56))

#define HT_SIPROUND                                                            \
    do {                                                                       \
        v0 += v1;                                                              \
        v1 = HT_SIP_ROTL(v1, 13);                                              \
        v1 ^= v0;                                                              \
        v0 = HT_SIP_ROTL(v0, 32);                                              \
        v2 += v3;                                                              \
        v3 = HT_SIP_ROTL(v3, 16);                                              \
        v3 ^= v2;                                                              \
        v0 += v3;                                                              \
        v3 = HT_SIP_ROTL(v3, 21);                                              \
        v3 ^= v0;                                                              \
        v2 += v1;                                                              \
        v1 = HT_SIP_ROTL(v1, 17);                                              \
        v1 ^= v2;                                                              \
        v2 = HT_SIP_ROTL(v2, 32);                                              \
    } while (0)

/// Compute a 64-bit SipHash-cROUNDS-dROUNDS of in[0..inlen) under the 16-byte
/// key k.
static inline uint64_t ht_siphash(const void *in, size_t inlen, const void *k,
                                  int cROUNDS, int dROUNDS) {
    const unsigned char *ni = (const unsigned char *)in;
    const unsigned char *kk = (const unsigned char *)k;

    uint64_t v0 = UINT64_C(0x736f6d6570736575);
    uint64_t v1 = UINT64_C(0x646f72616e646f6d);
    uint64_t v2 = UINT64_C(0x6c7967656e657261);
    uint64_t v3 = UINT64_C(0x7465646279746573);
    uint64_t k0 = HT_SIP_U8TO64_LE(kk);
    uint64_t k1 = HT_SIP_U8TO64_LE(kk + 8);
    uint64_t m;
    int i;
    const unsigned char *end = ni + inlen - (inlen % sizeof(uint64_t));
    const int left = inlen & 7;
    uint64_t b = ((uint64_t)inlen) << 56;

    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    for (; ni != end; ni += 8) {
        m = HT_SIP_U8TO64_LE(ni);
        v3 ^= m;
        for (i = 0; i < cROUNDS; ++i) {
            HT_SIPROUND;
        }
        v0 ^= m;
    }

    switch (left) {
    case 7:
        b |= ((uint64_t)ni[6]) << 48;
        /* fall through */
    case 6:
        b |= ((uint64_t)ni[5]) << 40;
        /* fall through */
    case 5:
        b |= ((uint64_t)ni[4]) << 32;
        /* fall through */
    case 4:
        b |= ((uint64_t)ni[3]) << 24;
        /* fall through */
    case 3:
        b |= ((uint64_t)ni[2]) << 16;
        /* fall through */
    case 2:
        b |= ((uint64_t)ni[1]) << 8;
        /* fall through */
    case 1:
        b |= ((uint64_t)ni[0]);
        break;
    case 0:
        break;
    }

    v3 ^= b;
    for (i = 0; i < cROUNDS; ++i) {
        HT_SIPROUND;
    }
    v0 ^= b;

    v2 ^= 0xff;
    for (i = 0; i < dROUNDS; ++i) {
        HT_SIPROUND;
    }

    return v0 ^ v1 ^ v2 ^ v3;
}

#endif /* HT_SIPHASH_H */

/* wyhash.h - Portable subset of wyhash, vendored.
 *
 * Source:    https://github.com/wangyi-fudan/wyhash (wyhash final v4.3)
 * Commit:    2ac9a50379bab7d6dab434e63dd1bad276791623
 * Retrieved: 2026-06-05
 * Author:    Wang Yi <godspeed_china@yeah.net> and contributors.
 * License:   public domain under The Unlicense (http://unlicense.org/); the
 *            full text is reproduced at the end of this file.
 *
 * This is the portable subset used by libhashtable. The core wyhash() function
 * and its read paths are copied verbatim; only the portable 64x64 -> 128
 * multiply branch is kept. The __uint128_t, MSVC _umul128, and 32-bit-MUM
 * multiply variants and the unused PRNG, secret-generation, and primality
 * helpers from upstream are omitted, and the __builtin_expect branch hints are
 * dropped. The portable multiply computes the same 128-bit product as the
 * __uint128_t path, so results match the upstream reference vectors.
 */

#ifndef HT_WYHASH_H
#define HT_WYHASH_H

#include <stdint.h>
#include <string.h>

/* Portable 64x64 -> 128 multiply mix (WYHASH_CONDOM == 1, WYHASH_32BIT_MUM
 * == 0). */
static inline uint64_t _wyrot(uint64_t x) { return (x >> 32) | (x << 32); }
static inline void _wymum(uint64_t *A, uint64_t *B) {
    uint64_t ha = *A >> 32, hb = *B >> 32, la = (uint32_t)*A, lb = (uint32_t)*B,
             hi, lo;
    uint64_t rh = ha * hb, rm0 = ha * lb, rm1 = hb * la, rl = la * lb,
             t = rl + (rm0 << 32), c = t < rl;
    lo = t + (rm1 << 32);
    c += lo < t;
    hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;
    *A = lo;
    *B = hi;
}
static inline uint64_t _wymix(uint64_t A, uint64_t B) {
    _wymum(&A, &B);
    return A ^ B;
}

/* endian detection */
#ifndef WYHASH_LITTLE_ENDIAN
#if defined(_WIN32) || defined(__LITTLE_ENDIAN__) ||                           \
    (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define WYHASH_LITTLE_ENDIAN 1
#elif defined(__BIG_ENDIAN__) ||                                               \
    (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define WYHASH_LITTLE_ENDIAN 0
#else
#define WYHASH_LITTLE_ENDIAN 1
#endif
#endif

/* read functions */
#if (WYHASH_LITTLE_ENDIAN)
static inline uint64_t _wyr8(const uint8_t *p) {
    uint64_t v;
    memcpy(&v, p, 8);
    return v;
}
static inline uint64_t _wyr4(const uint8_t *p) {
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}
#else
static inline uint64_t _wyr8(const uint8_t *p) {
    uint64_t v;
    memcpy(&v, p, 8);
    return (((v >> 56) & 0xff) | ((v >> 40) & 0xff00) | ((v >> 24) & 0xff0000) |
            ((v >> 8) & 0xff000000) | ((v << 8) & 0xff00000000) |
            ((v << 24) & 0xff0000000000) | ((v << 40) & 0xff000000000000) |
            ((v << 56) & 0xff00000000000000));
}
static inline uint64_t _wyr4(const uint8_t *p) {
    uint32_t v;
    memcpy(&v, p, 4);
    return (((v >> 24) & 0xff) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) |
            ((v << 24) & 0xff000000));
}
#endif
static inline uint64_t _wyr3(const uint8_t *p, size_t k) {
    return (((uint64_t)p[0]) << 16) | (((uint64_t)p[k >> 1]) << 8) | p[k - 1];
}

/* wyhash main function */
static inline uint64_t wyhash(const void *key, size_t len, uint64_t seed,
                              const uint64_t *secret) {
    const uint8_t *p = (const uint8_t *)key;
    seed ^= _wymix(seed ^ secret[0], secret[1]);
    uint64_t a, b;
    if (len <= 16) {
        if (len >= 4) {
            a = (_wyr4(p) << 32) | _wyr4(p + ((len >> 3) << 2));
            b = (_wyr4(p + len - 4) << 32) |
                _wyr4(p + len - 4 - ((len >> 3) << 2));
        } else if (len > 0) {
            a = _wyr3(p, len);
            b = 0;
        } else {
            a = b = 0;
        }
    } else {
        size_t i = len;
        if (i >= 48) {
            uint64_t see1 = seed, see2 = seed;
            do {
                seed = _wymix(_wyr8(p) ^ secret[1], _wyr8(p + 8) ^ seed);
                see1 = _wymix(_wyr8(p + 16) ^ secret[2], _wyr8(p + 24) ^ see1);
                see2 = _wymix(_wyr8(p + 32) ^ secret[3], _wyr8(p + 40) ^ see2);
                p += 48;
                i -= 48;
            } while (i >= 48);
            seed ^= see1 ^ see2;
        }
        while (i > 16) {
            seed = _wymix(_wyr8(p) ^ secret[1], _wyr8(p + 8) ^ seed);
            i -= 16;
            p += 16;
        }
        a = _wyr8(p + i - 16);
        b = _wyr8(p + i - 8);
    }
    a ^= secret[1];
    b ^= seed;
    _wymum(&a, &b);
    return _wymix(a ^ secret[0] ^ len, b ^ secret[1]);
}

/* the default secret parameters */
static const uint64_t _wyp[4] = {0x2d358dccaa6c78a5ull, 0x8bb84b93962eacc9ull,
                                 0x4b33a62ed433d4a3ull, 0x4d5a2da51de1aa47ull};

#endif /* HT_WYHASH_H */

/* The Unlicense
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or distribute
 * this software, either in source code form or as a compiled binary, for any
 * purpose, commercial or non-commercial, and by any means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors of
 * this software dedicate any and all copyright interest in the software to the
 * public domain. We make this dedication for the benefit of the public at
 * large and to the detriment of our heirs and successors. We intend this
 * dedication to be an overt act of relinquishment in perpetuity of all present
 * and future rights to this software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

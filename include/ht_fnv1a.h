/* ht_fnv1a.h - FNV1A hash algorithm implementation.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <stdbool.h>

#ifndef __HT_FNV1A_H__
#define __HT_FNV1A_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CPU_32_BIT)

#define FNV1A_PRIME (0x01000193)  // 16777619 (32 bit)
#define FNV1A_OFFSET (0x811C9DC5) // 2166136261 (32 bit)

uint32_t fnv1a_hash_str(const void *, uint32_t);
uint32_t fnv1a_hash_str_casecmp(const void *, uint32_t);

#elif defined(CPU_64_BIT)

#define FNV1A_PRIME (0x00000100000001B3)  // 1099511628211 (64 bit)
#define FNV1A_OFFSET (0xCBF29CE484222325) // 14695981039346656037 (64 bit)

uint64_t fnv1a_hash_str(const void *, uint64_t);
uint64_t fnv1a_hash_str_casecmp(const void *, uint64_t);

#endif

bool str_eq(const void *, const void *);
bool str_caseeq(const void *, const void *);

#ifdef __cplusplus
}
#endif

#endif  // __HT_FNV1A_H__

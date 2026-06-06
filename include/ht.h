/* ht.h - A generic hash table implementation.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2024
 */

#ifndef __HT_H__
#define __HT_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ht ht_t;
typedef struct ht_enum ht_enum_t;
typedef struct ht_strblob ht_strblob_t;
typedef struct ht_strdouble ht_strdouble_t;
typedef struct ht_strfloat ht_strfloat_t;
typedef struct ht_strint ht_strint_t;
typedef struct ht_strstr ht_strstr_t;
typedef struct ht_u64ptr ht_u64ptr_t;
typedef struct ht_u64blob ht_u64blob_t;

typedef enum {
    HT_KEY_NONE = 0, ///< unkeyed hash; hashkey is NULL
    HT_KEY_RANDOM,   ///< per-table key generated from the CSPRNG
    HT_KEY_PROVIDED, ///< caller supplies the 16-byte key material
} ht_key_mode;

/* Hash values are unconditionally 64-bit. */
typedef uint64_t ht_hash_t;
#define FNV1A_PRIME (0x00000100000001B3ull)  ///< 1099511628211
#define FNV1A_OFFSET (0xCBF29CE484222325ull) ///< 14695981039346656037

typedef ht_hash_t (*ht_hash)(const void *, size_t, const void *);
typedef bool (*ht_keyeq)(const void *, const void *);
typedef size_t (*ht_keylen)(const void *);
typedef void *(*ht_kcopy)(const void *);
typedef void (*ht_kfree)(const void *);
typedef void *(*ht_vcopy)(const void *);
typedef void (*ht_vfree)(const void *);
typedef void (*ht_visit)(const void *, const void *, void *);

typedef struct {
    ht_kcopy key_copy;
    ht_kfree key_free;
    ht_vcopy val_copy;
    ht_vfree val_free;
} ht_callbacks_t;

/// Construction options. Zero-initialized fields take defaults: passthrough
/// callbacks, HT_KEY_NONE, and the default initial capacity.
typedef struct {
    ht_hash hash;             ///< required
    ht_keyeq keyeq;           ///< required
    ht_keylen keylen;         ///< required
    ht_callbacks_t callbacks; ///< zero-initialized => passthrough
    ht_key_mode key_mode;     ///< default HT_KEY_NONE
    const void *key;          ///< HT_KEY_PROVIDED: 16 bytes of key material
    bool key_best_effort; ///< HT_KEY_RANDOM: degrade vs fail on CSPRNG failure
    size_t initial_capacity; ///< 0 => default
} ht_options_t;

/// Construction options for the string-keyed typed wrappers. case_insensitive
/// and flooding_resistant are mutually exclusive (SipHash has no case-folding
/// variant); requesting both fails the create. Zero-initialized => a
/// case-sensitive, unkeyed FNV-1a table at the default capacity.
typedef struct {
    bool case_insensitive;   ///< case-insensitive keys (FNV-1a casecmp)
    bool flooding_resistant; ///< keyed SipHash with a random per-table key
    bool best_effort;        ///< flooding_resistant: degrade vs fail
    size_t initial_capacity; ///< 0 => default
} ht_str_options_t;

/// Construction options for the integer-keyed (uint64) typed wrappers.
/// flooding_resistant selects keyed SipHash over the key bytes for adversarial
/// keys; otherwise the integer finalizer is used. Zero-initialized => an
/// unkeyed table at the default capacity.
typedef struct {
    bool flooding_resistant; ///< keyed SipHash with a random per-table key
    bool best_effort;        ///< flooding_resistant: degrade vs fail
    size_t initial_capacity; ///< 0 => default
} ht_u64_options_t;

ht_hash_t ht_hash_fnv1a(const void *, size_t, const void *);
ht_hash_t ht_hash_fnv1a_casecmp(const void *, size_t, const void *);

/// Integer-key hash via the splitmix64 (Stafford Mix13) finalizer
ht_hash_t ht_hash_int(const void *, size_t, const void *);

/// Bulk hash (wyhash) for long string and binary keys
ht_hash_t ht_hash_bulk(const void *, size_t, const void *);

/// Keyed SipHash for adversarial input; hashkey must point to a 16-byte key.
/// ht_hash_siphash is SipHash-1-3 (faster default); ht_hash_siphash24 is
/// SipHash-2-4 (conservative).
ht_hash_t ht_hash_siphash(const void *, size_t, const void *);
ht_hash_t ht_hash_siphash24(const void *, size_t, const void *);

/// String key equality functions
bool str_eq(const void *, const void *);
bool str_caseeq(const void *, const void *);

/// String key length function
size_t str_len(const void *);

/// Fill the first len bytes of buf with cryptographically strong random data.
/// Returns 0 on success, -1 on failure. On failure buf is unusable and a weak
/// entropy source is never substituted.
int ht_random_bytes(void *buf, size_t len);

// Creation and destruction
ht_t *ht_create(const ht_options_t *);
void ht_destroy(ht_t *);
ht_strblob_t *ht_strblob_create(const ht_str_options_t *);
void ht_strblob_destroy(ht_strblob_t *);
ht_strdouble_t *ht_strdouble_create(const ht_str_options_t *);
void ht_strdouble_destroy(ht_strdouble_t *);
ht_strfloat_t *ht_strfloat_create(const ht_str_options_t *);
void ht_strfloat_destroy(ht_strfloat_t *);
ht_strint_t *ht_strint_create(const ht_str_options_t *);
void ht_strint_destroy(ht_strint_t *);
ht_strstr_t *ht_strstr_create(const ht_str_options_t *);
void ht_strstr_destroy(ht_strstr_t *);
ht_u64ptr_t *ht_u64ptr_create(const ht_u64_options_t *);
void ht_u64ptr_destroy(ht_u64ptr_t *);
ht_u64blob_t *ht_u64blob_create(const ht_u64_options_t *);
void ht_u64blob_destroy(ht_u64blob_t *);

// Insertion and removal
void ht_insert(ht_t *, const void *, const void *);
void ht_remove(ht_t *, const void *);
void ht_strblob_insert(ht_strblob_t *, const char *, const void *, size_t);
void ht_strblob_remove(ht_strblob_t *, const char *);
void ht_strdouble_insert(ht_strdouble_t *, const char *, const double *);
void ht_strdouble_remove(ht_strdouble_t *, const char *);
void ht_strfloat_insert(ht_strfloat_t *, const char *, const float *);
void ht_strfloat_remove(ht_strfloat_t *, const char *);
void ht_strint_insert(ht_strint_t *, const char *, const int *);
void ht_strint_remove(ht_strint_t *, const char *);
void ht_strstr_insert(ht_strstr_t *, const char *, const char *);
void ht_strstr_remove(ht_strstr_t *, const char *);
void ht_u64ptr_insert(ht_u64ptr_t *, uint64_t, void *);
void ht_u64ptr_remove(ht_u64ptr_t *, uint64_t);
void ht_u64blob_insert(ht_u64blob_t *, uint64_t, const void *, size_t);
void ht_u64blob_remove(ht_u64blob_t *, uint64_t);

// Getting
void *ht_get(const ht_t *, const void *);
const void *ht_strblob_get(ht_strblob_t *, const char *, size_t *);
void *ht_strdouble_get(ht_strdouble_t *, const char *);
void *ht_strfloat_get(ht_strfloat_t *, const char *);
void *ht_strint_get(ht_strint_t *, const char *);
const char *ht_strstr_get(ht_strstr_t *, const char *);
void *ht_u64ptr_get(ht_u64ptr_t *, uint64_t);
const void *ht_u64blob_get(ht_u64blob_t *, uint64_t, size_t *);

/* Size, membership, and bulk operations */
size_t ht_size(const ht_t *);
bool ht_contains(const ht_t *, const void *);
void *ht_get_or_insert(ht_t *, const void *, const void *);
bool ht_upsert(ht_t *, const void *, const void *);
void ht_clear(ht_t *);
void ht_foreach(const ht_t *, ht_visit, void *);

// Enumeration
ht_enum_t *ht_enum_create(ht_t *);
bool ht_enum_next(ht_enum_t *, const void **, const void **);
void ht_enum_destroy(ht_enum_t *);
ht_enum_t *ht_strblob_enum_create(ht_strblob_t *);
bool ht_strblob_enum_next(ht_enum_t *, const char **, const void **, size_t *);
void ht_strblob_enum_destroy(ht_enum_t *);
ht_enum_t *ht_strdouble_enum_create(ht_strdouble_t *);
bool ht_strdouble_enum_next(ht_enum_t *, const char **, const double **);
void ht_strdouble_enum_destroy(ht_enum_t *);
ht_enum_t *ht_strfloat_enum_create(ht_strfloat_t *);
bool ht_strfloat_enum_next(ht_enum_t *, const char **, const float **);
void ht_strfloat_enum_destroy(ht_enum_t *);
ht_enum_t *ht_strint_enum_create(ht_strint_t *);
bool ht_strint_enum_next(ht_enum_t *, const char **, const int **);
void ht_strint_enum_destroy(ht_enum_t *);
ht_enum_t *ht_strstr_enum_create(ht_strstr_t *);
bool ht_strstr_enum_next(ht_enum_t *, const char **, const char **);
void ht_strstr_enum_destroy(ht_enum_t *);
ht_enum_t *ht_u64ptr_enum_create(ht_u64ptr_t *);
bool ht_u64ptr_enum_next(ht_enum_t *, uint64_t *, void **);
void ht_u64ptr_enum_destroy(ht_enum_t *);
ht_enum_t *ht_u64blob_enum_create(ht_u64blob_t *);
bool ht_u64blob_enum_next(ht_enum_t *, uint64_t *, const void **, size_t *);
void ht_u64blob_enum_destroy(ht_enum_t *);

#ifdef __cplusplus
}
#endif

#endif // __HT_H__

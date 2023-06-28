/* ht.h - A generic hash table implementation.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#ifndef __HT_H__
#define __HT_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ht ht_t;
typedef struct ht_enum ht_enum_t;
typedef struct ht_strdouble ht_strdouble_t;
typedef struct ht_strfloat ht_strfloat_t;
typedef struct ht_strint ht_strint_t;
typedef struct ht_strstr ht_strstr_t;

typedef enum {
    HT_STR_NONE = 0,
    HT_STR_CASECMP,
    HT_SEED_RANDOM,
} ht_flags_enum_t;

#if defined(CPU_32_BIT)
typedef uint32_t (*ht_hash)(const void *, uint32_t);
#elif defined(CPU_64_BIT)
typedef uint64_t (*ht_hash)(const void *, uint64_t);
#endif
typedef bool (*ht_keyeq)(const void *, const void *);
typedef void *(*ht_kcopy)(void *);
typedef void (*ht_kfree)(void *);
typedef void *(*ht_vcopy)(void *);
typedef void (*ht_vfree)(void *);

typedef struct {
    ht_kcopy key_copy;
    ht_kfree key_free;
    ht_vcopy val_copy;
    ht_vfree val_free;
} ht_callbacks_t;

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

// String key equality functinos
bool str_eq(const void *, const void *);
bool str_caseeq(const void *, const void *);

// Creation and destruction
ht_t *ht_create(ht_hash, ht_keyeq, ht_callbacks_t *, unsigned int);
void ht_destroy(ht_t *);
ht_strdouble_t *ht_strdouble_create(unsigned int);
void ht_strdouble_destroy(ht_strdouble_t *);
ht_strfloat_t *ht_strfloat_create(unsigned int);
void ht_strfloat_destroy(ht_strfloat_t *);
ht_strint_t *ht_strint_create(unsigned int);
void ht_strint_destroy(ht_strint_t *);
ht_strstr_t *ht_strstr_create(unsigned int);
void ht_strstr_destroy(ht_strstr_t *);

// Insertion and removal
void ht_insert(ht_t *, void *, void *);
void ht_remove(ht_t *, void *);
void ht_strdouble_insert(ht_strdouble_t *, const char *, const double *);
void ht_strdouble_remove(ht_strdouble_t *, const char *);
void ht_strfloat_insert(ht_strfloat_t *, const char *, const float *);
void ht_strfloat_remove(ht_strfloat_t *, const char *);
void ht_strint_insert(ht_strint_t *, const char *, const int *);
void ht_strint_remove(ht_strint_t *, const char *);
void ht_strstr_insert(ht_strstr_t *, const char *, const char *);
void ht_strstr_remove(ht_strstr_t *, const char *);

// Getting
void *ht_get(ht_t *, void *);
void *ht_strdouble_get(ht_strdouble_t *, const char *);
void *ht_strfloat_get(ht_strfloat_t *, const char *);
void *ht_strint_get(ht_strint_t *, const char *);
const char *ht_strstr_get(ht_strstr_t *, const char *);

// Enumeration
ht_enum_t *ht_enum_create(ht_t *);
bool ht_enum_next(ht_enum_t *, void **, void **);
void ht_enum_destroy(ht_enum_t *);
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

#ifdef __cplusplus
}
#endif

#endif  // __HT_H__

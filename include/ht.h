/* ht.h - A generic hash table implementation.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <stdint.h>
#include <stdbool.h>

#ifndef __HT_H__
#define __HT_H__

typedef struct ht ht_t;
typedef struct ht_enum ht_enum_t;

#if defined(CPU_32_BIT)
typedef uint32_t (*ht_hash)(const void *);
#elif defined(CPU_64_BIT)
typedef uint64_t (*ht_hash)(const void *);
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

// Creation and destruction
ht_t *ht_create(ht_hash, ht_keyeq, ht_callbacks_t *);
void ht_destroy(ht_t *);

// Insertion and removal
void ht_insert(ht_t *, void *, void *);
void ht_remove(ht_t *, void *);

// Getting
bool ht_get(ht_t *, void *, void **);
void *ht_get_direct(ht_t *, void *);

// Enumeration
ht_enum_t *ht_enum_create(ht_t *);
bool ht_enum_next(ht_enum_t*, void **, void **);
void ht_enum_destroy(ht_enum_t*);

#endif

/* ht_strfloat.h - Type wrapped implementation of a string->float hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <stdbool.h>

#ifndef __HT_STRFLOAT_H__
#define __HT_STRFLOAT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ht_strfloat ht_strfloat_t;
typedef struct ht_strfloat_enum ht_strfloat_enum_t;

typedef enum {
    HT_STR_NONE = 0,
    HT_STR_CASECMP,
} ht_strfloat_flags_enum_t;

// Creation and destruction
ht_strfloat_t *ht_strfloat_create(unsigned int);
void ht_strfloat_destroy(ht_strfloat_t *);

// Insertion and removal
void ht_strfloat_insert(ht_strfloat_t *, const char *, const float *);
void ht_strfloat_remove(ht_strfloat_t *, const char *);

// Getting
bool ht_strfloat_get(ht_strfloat_t *, const char *, const float **);
void *ht_strfloat_get_direct(ht_strfloat_t *, const char *);

// Enumeration
ht_strfloat_enum_t *ht_strfloat_enum_create(ht_strfloat_t *);
bool ht_strfloat_enum_next(ht_strfloat_enum_t *, const char **, const float **);
void ht_strfloat_enum_destroy(ht_strfloat_enum_t *);

#ifdef __cplusplus
}
#endif

#endif  // __HT_STRFLOAT_H__

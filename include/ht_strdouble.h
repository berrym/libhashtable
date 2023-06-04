/* ht_strdouble.h - Type wrapped implementation of a string->double hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <stdbool.h>

#ifndef __HT_STRDOUBLE_H__
#define __HT_STRDOUBLE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ht_strdouble ht_strdouble_t;
typedef struct ht_strdouble_enum ht_strdouble_enum_t;

typedef enum {
    HT_STR_NONE = 0,
    HT_STR_CASECMP,
} ht_strdouble_flags_enum_t;

// Creation and destruction
ht_strdouble_t *ht_strdouble_create(unsigned int);
void ht_strdouble_destroy(ht_strdouble_t *);

// Insertion and removal
void ht_strdouble_insert(ht_strdouble_t *, const char *, const double *);
void ht_strdouble_remove(ht_strdouble_t *, const char *);

// Getting
bool ht_strdouble_get(ht_strdouble_t *, const char *, const double **);
void *ht_strdouble_get_direct(ht_strdouble_t *, const char *);

// Enumeration
ht_strdouble_enum_t *ht_strdouble_enum_create(ht_strdouble_t *);
bool ht_strdouble_enum_next(ht_strdouble_enum_t *, const char **, const double **);
void ht_strdouble_enum_destroy(ht_strdouble_enum_t *);

#ifdef __cplusplus
}
#endif

#endif  // __HT_STRDOUBLE_H__

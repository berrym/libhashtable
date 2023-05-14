/* ht_strint.h - Type wrapped implementation of a string->int hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <stdbool.h>

#ifndef __HT_STRINT_H__
#define __HT_STRINT_H__

typedef struct ht_strint ht_strint_t;
typedef struct ht_strint_enum ht_strint_enum_t;

typedef enum {
    HT_STR_NONE = 0,
    HT_STR_CASECMP
} ht_strint_flags_enum_t;

// Creation and destruction
ht_strint_t *ht_strint_create(unsigned int);
void ht_strint_destroy(ht_strint_t *);

// Insertion and removal
void ht_strint_insert(ht_strint_t *, const char *, const int *);
void ht_strint_remove(ht_strint_t *, const char *);

// Getting
bool ht_strint_get(ht_strint_t *, const char *, const int **);
void *ht_strint_get_direct(ht_strint_t *, const char *);

// Enumeration
ht_strint_enum_t *ht_strint_enum_create(ht_strint_t *);
bool ht_strint_enum_next(ht_strint_enum_t *, const char **, const int **);
void ht_strint_enum_destroy(ht_strint_enum_t *);

#endif

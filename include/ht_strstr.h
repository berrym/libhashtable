/* ht_strstr.h - Type wrapped implementation of a string->string hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <stdbool.h>

#ifndef __HT_STRSTR_H__
#define __HT_STRSTR_H__

typedef struct ht_strstr ht_strstr_t;
typedef struct ht_strstr_enum ht_strstr_enum_t;

typedef enum {
    HT_STR_NONE = 0,
    HT_STR_CASECMP,
} ht_strstr_flags_enum_t;

// Creation and destruction
ht_strstr_t *ht_strstr_create(unsigned int);
void ht_strstr_destroy(ht_strstr_t *);

// Insertion and removal
void ht_strstr_insert(ht_strstr_t *, const char *, const char *);
void ht_strstr_remove(ht_strstr_t *, const char *);

// Getting
bool ht_strstr_get(ht_strstr_t *, const char *, const char **);
const char *ht_strstr_get_direct(ht_strstr_t *, const char *);

// Enumeration
ht_strstr_enum_t *ht_strstr_enum_create(ht_strstr_t *);
bool ht_strstr_enum_next(ht_strstr_enum_t *, const char **, const char **);
void ht_strstr_enum_destroy(ht_strstr_enum_t *);

#endif

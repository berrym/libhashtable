/* ht_strdouble_test.c - Test program for string->double hashtable.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include "ht.h"                 // for HT_SEED_RANDOM
#include "ht_strdouble.h"

int main(int argc, char **argv)
{
    ht_strdouble_t *ht = NULL;

    ht = ht_strdouble_create(HT_STR_CASECMP | HT_SEED_RANDOM);
    if (!ht)
        exit(EXIT_FAILURE);

    double i = 123.456;
    const double *a = &i;
    const double *b;

    ht_strdouble_insert(ht, "Abc", a);
    b = ht_strdouble_get_direct(ht, "Abc");
    printf("%lf: %p, %p\n", *((double *)b), a, b);

    i = 456.789;
    ht_strdouble_insert(ht, "aBc", a);
    b = ht_strdouble_get_direct(ht, "Abc");
    printf("%lf: %p, %p\n", *((double *)b), a, b);

    ht_strdouble_remove(ht, "abC");

    size_t len = 20;
    char a_key[64] = { '\0' };
    double a_val = 0;

    for (size_t i = 0; i < len; i++) {
        snprintf(a_key, sizeof(a_key), "a%zu", i);
        a_val = (i * 100.5) + i + (i / 2.5);
        ht_strdouble_insert(ht, a_key, &a_val);
    }

    ht_strdouble_enum_t *he = NULL;

    he = ht_strdouble_enum_create(ht);
    if (!he) {
        ht_strdouble_destroy(ht);
        exit(EXIT_FAILURE);
    }

    const char *key = NULL;
    while (ht_strdouble_enum_next(he, &key, &b))
        printf("key=%s, val=%lf\n", key, *((double *)b));

    ht_strdouble_enum_destroy(he);
    ht_strdouble_destroy(ht);

    return 0;
}

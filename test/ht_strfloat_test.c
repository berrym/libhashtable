/* ht_strfloat_test.c - Test program for string->float hashtable.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include "ht.h"                 // for HT_SEED_RANDOM
#include "ht_strfloat.h"

int main(int argc, char **argv)
{
    ht_strfloat_t *ht = NULL;

    ht = ht_strfloat_create(HT_STR_CASECMP | HT_SEED_RANDOM);
    if (!ht)
        exit(EXIT_FAILURE);

    float i = 123.456;
    const float *a = &i;
    const float *b;

    ht_strfloat_insert(ht, "Abc", a);
    b = ht_strfloat_get_direct(ht, "Abc");
    printf("%f: %p, %p\n", *((float *)b), a, b);

    i = 456.789;
    ht_strfloat_insert(ht, "aBc", a);
    b = ht_strfloat_get_direct(ht, "Abc");
    printf("%f: %p, %p\n", *((float *)b), a, b);

    ht_strfloat_remove(ht, "abC");

    size_t len = 20;
    char a_key[64] = { '\0' };
    float a_val = 0;

    for (size_t i = 0; i < len; i++) {
        snprintf(a_key, sizeof(a_key), "a%zu", i);
        a_val = (i * 100.5) + i + (i / 2.5);
        ht_strfloat_insert(ht, a_key, &a_val);
    }

    ht_strfloat_enum_t *he = NULL;

    he = ht_strfloat_enum_create(ht);
    if (!he) {
        ht_strfloat_destroy(ht);
        exit(EXIT_FAILURE);
    }

    const char *key = NULL;
    while (ht_strfloat_enum_next(he, &key, &b))
        printf("key=%s, val=%f\n", key, *((float *)b));

    ht_strfloat_enum_destroy(he);
    ht_strfloat_destroy(ht);

    return 0;
}

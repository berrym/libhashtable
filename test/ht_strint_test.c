/* ht_strint_test.c - Test program for string->int hashtable.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include "ht.h"                 // for HT_SEED_RANDOM
#include "ht_strint.h"

int main(int argc, char **argv)
{
    ht_strint_t *ht = NULL;

    ht = ht_strint_create(HT_STR_CASECMP | HT_SEED_RANDOM);
    if (!ht)
        exit(EXIT_FAILURE);

    int i = 123;
    const int *a = &i;
    const int *b;

    ht_strint_insert(ht, "Abc", a);
    b = ht_strint_get_direct(ht, "Abc");
    printf("%d: %p, %p\n", *((int *)b), a, b);

    i = 456;
    ht_strint_insert(ht, "aBc", a);
    b = ht_strint_get_direct(ht, "Abc");
    printf("%d: %p, %p\n", *((int *)b), a, b);

    size_t len = 20;
    char a_key[64] = { '\0' };
    int a_val = 0;

    for (size_t i = 0; i < len; i++) {
        snprintf(a_key, sizeof(a_key), "a%zu", i);
        a_val = (i * 100) + i + (i / 2);
        ht_strint_insert(ht, a_key, &a_val);
    }

    ht_strint_enum_t *he = NULL;

    he = ht_strint_enum_create(ht);
    if (!he) {
        ht_strint_destroy(ht);
        exit(EXIT_FAILURE);
    }

    const char *key = NULL;
    while (ht_strint_enum_next(he, &key, &b))
        printf("key=%s, val=%d\n", key, *((int *)b));

    ht_strint_enum_destroy(he);
    ht_strint_destroy(ht);

    return 0;
}

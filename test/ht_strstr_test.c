/* ht_strstr_test.c - Test program for string->string hashtable.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2024
 */

#include "ht.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    ht_strstr_t *ht = NULL;
    ht_enum_t *he = NULL;
    const char *a = "apple";
    const char *b = NULL;
    const size_t len = 20;
    char t1[64] = {'\0'};
    char t2[64] = {'\0'};

    ht = ht_strstr_create(HT_STR_CASECMP | HT_SEED_RANDOM);
    if (!ht) {
        exit(EXIT_FAILURE);
    }

    ht_strstr_insert(ht, "Abc", a);
    b = ht_strstr_get(ht, "Abc");
    printf("%s: %p, %p\n", b, a, b);

    ht_strstr_insert(ht, "aBc", "orange");
    b = ht_strstr_get(ht, "Abc");
    printf("%s: %p, %p\n", b, a, b);

    ht_strstr_remove(ht, "abC");

    for (size_t i = 0; i < len; i++) {
        snprintf(t1, sizeof(t1), "a%zu", i);
        snprintf(t2, sizeof(t2), "%zu", (i * 100) + i + (i / 2));
        ht_strstr_insert(ht, t1, t2);
    }

    he = ht_strstr_enum_create(ht);
    if (!he) {
        ht_strstr_destroy(ht);
        exit(EXIT_FAILURE);
    }

    while (ht_strstr_enum_next(he, &a, &b)) {
        printf("key=%s, val=%s\n", a, b);
    }

    ht_strstr_enum_destroy(he);
    ht_strstr_destroy(ht);

    return 0;
}

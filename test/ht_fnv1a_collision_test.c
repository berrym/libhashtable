/* ht_strstr_test.c - Test program for string->string hashtable.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include "ht.h"

int main(int argc, char **argv)
{
    ht_strstr_t *ht = NULL;
    ht_enum_t *he = NULL;
    const char *a = NULL;
    const char *b = NULL;
    const size_t len = 20;
    char t1[64] = { '\0' };
    char t2[64] = { '\0' };

    ht = ht_strstr_create(HT_STR_NONE);
    if (!ht)
        exit(EXIT_FAILURE);

#if defined(CPU_32_BIT)

    // Create a collision - 32 bit hash
    ht_strstr_insert(ht, "costarring", "apple");
    ht_strstr_insert(ht, "liquid", "orange");

    ht_strstr_remove(ht, "costarring");

    // Create another collision - 32 bit hash
    ht_strstr_insert(ht, "declinate", "banana");
    ht_strstr_insert(ht, "macallums", "pineapple");

    // Create another collision - 32 bit hash
    ht_strstr_insert(ht, "altarage", "strawberry");
    ht_strstr_insert(ht, "zinke", "mango");

    // Create another collision - 32 bit hash
    ht_strstr_insert(ht, "altarages", "pear");
    ht_strstr_insert(ht, "zinkes", "grape");

#elif defined(CPU_64_BIT)

    // Create a collision - 64 bit hash (0x4EAC0C95540867E4)
    ht_strstr_insert(ht, "8yn0iYCKYHlIj4-BwPqk", "apple");
    ht_strstr_insert(ht, "GReLUrM4wMqfg9yzV3KQ", "orange");

    ht_strstr_remove(ht, "8yn0iYCKYHlIj4-BwPqk");

    // Create another collision - 64 bit hash (0x8FCF3BE2DE898214)
    ht_strstr_insert(ht, "gMPflVXtwGDXbIhP73TX", "banana");
    ht_strstr_insert(ht, "LtHf1prlU1bCeYZEdqWf", "pineapple");

    // Create another collision - 64 bit hash (0xF8893E25250C0EAA)
    ht_strstr_insert(ht, "pFuM83THhM-Qw8FI5FKo", "strawberry");
    ht_strstr_insert(ht, ".jPx7rOtTDteKAwvfOEo", "mango");

    // Create another collision - 64 bit hash (0x184ADE9A961953A2)
    ht_strstr_insert(ht, "7mohtcOFVz", "pear");
    ht_strstr_insert(ht, "c1E51sSEyx", "grape");

    // Create another collision - 64 bit hash (0x703C4DD295D7AA15)
    ht_strstr_insert(ht, "6a5x-VbtXk", "blackberry");
    ht_strstr_insert(ht, "f_2k7GG-4v", "raspberry");

#endif

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

    while (ht_strstr_enum_next(he, &a, &b))
        printf("key=%s, val=%s\n", a, b);

    ht_strstr_enum_destroy(he);
    ht_strstr_destroy(ht);

    return 0;
}

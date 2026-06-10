/**
 * @file ht_functional_test.c
 * @brief Functional unit tests for the full public API.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 *
 * Exercises the generic core, the keyed-table paths, user_data forwarding,
 * statistics, insertion ordering, every typed wrapper, and the hash-algorithm
 * contracts. Each test asserts observable behavior rather than printing.
 */

#include "ht.h"
#include "ht_test.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// String key/value lifetime callbacks for base-layer tables.
static const ht_callbacks_t STR_CB = {str_copy, str_free, str_copy, str_free};

/// A hash that forces every key into one bucket, to drive collision chains.
static ht_hash_t all_collide(const void *key, size_t len, const void *hashkey) {
    (void)key;
    (void)len;
    (void)hashkey;
    return 0;
}

/// Deterministic generator so data-heavy tests are reproducible.
static uint64_t splitmix64(uint64_t *s) {
    uint64_t z = (*s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static ht_t *new_str_table(void) {
    return ht_create(&(ht_options_t){.hash = ht_hash_fnv1a,
                                     .keyeq = str_eq,
                                     .keylen = str_len,
                                     .callbacks = STR_CB});
}

/* ---- generic core ---- */

static void test_insert_get(void) {
    ht_t *ht = new_str_table();
    ASSERT_NOT_NULL(ht);
    ht_insert(ht, "alpha", "one");
    ht_insert(ht, "beta", "two");
    ASSERT_STR_EQ(ht_get(ht, "alpha"), "one");
    ASSERT_STR_EQ(ht_get(ht, "beta"), "two");
    ht_destroy(ht);
}

static void test_get_absent(void) {
    ht_t *ht = new_str_table();
    ht_insert(ht, "present", "x");
    ASSERT_NULL(ht_get(ht, "absent"));
    ht_destroy(ht);
}

static void test_contains_distinguishes_null_value(void) {
    ht_t *ht = new_str_table();
    ht_insert(ht, "haskey", NULL);
    ASSERT_TRUE(ht_contains(ht, "haskey"));
    ASSERT_NULL(ht_get(ht, "haskey"));
    ASSERT_FALSE(ht_contains(ht, "nope"));
    ht_destroy(ht);
}

static void test_replace_value(void) {
    ht_t *ht = new_str_table();
    ht_insert(ht, "k", "first");
    ht_insert(ht, "k", "second");
    ASSERT_STR_EQ(ht_get(ht, "k"), "second");
    ASSERT_EQ_UINT(ht_size(ht), 1);
    ht_destroy(ht);
}

static void test_size_tracking(void) {
    ht_t *ht = new_str_table();
    ASSERT_EQ_UINT(ht_size(ht), 0);
    ht_insert(ht, "a", "1");
    ht_insert(ht, "b", "2");
    ht_insert(ht, "c", "3");
    ASSERT_EQ_UINT(ht_size(ht), 3);
    ht_insert(ht, "a", "1again"); /* replace, not new */
    ASSERT_EQ_UINT(ht_size(ht), 3);
    ht_remove(ht, "b");
    ASSERT_EQ_UINT(ht_size(ht), 2);
    ht_destroy(ht);
}

static void test_remove(void) {
    ht_t *ht = new_str_table();
    ht_insert(ht, "x", "1");
    ht_insert(ht, "y", "2");
    ht_remove(ht, "x");
    ASSERT_NULL(ht_get(ht, "x"));
    ASSERT_STR_EQ(ht_get(ht, "y"), "2");
    ht_remove(ht, "absent"); /* no-op, must not corrupt */
    ASSERT_EQ_UINT(ht_size(ht), 1);
    ht_destroy(ht);
}

static void test_upsert(void) {
    ht_t *ht = new_str_table();
    ASSERT_TRUE(ht_upsert(ht, "k", "v1"));  /* newly added */
    ASSERT_FALSE(ht_upsert(ht, "k", "v2")); /* replaced */
    ASSERT_STR_EQ(ht_get(ht, "k"), "v2");
    ht_destroy(ht);
}

static void test_get_or_insert(void) {
    ht_t *ht = new_str_table();
    const char *v = ht_get_or_insert(ht, "k", "orig");
    ASSERT_STR_EQ(v, "orig");
    v = ht_get_or_insert(ht, "k", "ignored");
    ASSERT_STR_EQ(v, "orig"); /* existing value, not overwritten */
    ASSERT_EQ_UINT(ht_size(ht), 1);
    ht_destroy(ht);
}

static void test_clear_then_reuse(void) {
    ht_t *ht = new_str_table();
    for (int i = 0; i < 50; i++) {
        char k[16];
        snprintf(k, sizeof(k), "k%d", i);
        ht_insert(ht, k, "v");
    }
    ht_clear(ht);
    ASSERT_EQ_UINT(ht_size(ht), 0);
    ASSERT_FALSE(ht_contains(ht, "k0"));
    ht_insert(ht, "fresh", "ok");
    ASSERT_EQ_UINT(ht_size(ht), 1);
    ASSERT_STR_EQ(ht_get(ht, "fresh"), "ok");
    ht_destroy(ht);
}

/// Visitor that records, per parsed index, how many times a key was seen.
static void mark_visit(const void *key, const void *val, void *user_data) {
    (void)val;
    int *seen = user_data;
    int idx = -1;
    if (sscanf((const char *)key, "k%d", &idx) == 1 && idx >= 0 && idx < 100) {
        seen[idx]++;
    }
}

static void test_foreach_visits_each_once(void) {
    ht_t *ht = new_str_table();
    for (int i = 0; i < 100; i++) {
        char k[16];
        snprintf(k, sizeof(k), "k%d", i);
        ht_insert(ht, k, "v");
    }
    int seen[100] = {0};
    ht_foreach(ht, mark_visit, seen);
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ_INT(seen[i], 1);
    }
    ht_destroy(ht);
}

static void test_enum_visits_each_once(void) {
    ht_t *ht = new_str_table();
    for (int i = 0; i < 100; i++) {
        char k[16];
        snprintf(k, sizeof(k), "k%d", i);
        ht_insert(ht, k, "v");
    }
    int seen[100] = {0};
    ht_enum_t *he = ht_enum_create(ht);
    ASSERT_NOT_NULL(he);
    const void *key = NULL;
    size_t count = 0;
    while (ht_enum_next(he, &key, NULL)) {
        int idx = -1;
        ASSERT_EQ_INT(sscanf((const char *)key, "k%d", &idx), 1);
        ASSERT_TRUE(idx >= 0 && idx < 100);
        seen[idx]++;
        count++;
    }
    ht_enum_destroy(he);
    ASSERT_EQ_UINT(count, 100);
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ_INT(seen[i], 1);
    }
    ht_destroy(ht);
}

static void test_deep_copy_independent_of_caller_buffers(void) {
    ht_t *ht = new_str_table();
    char kbuf[16];
    char vbuf[16];
    strcpy(kbuf, "mykey");
    strcpy(vbuf, "myval");
    ht_insert(ht, kbuf, vbuf);

    /* Mutating the caller's buffers must not affect stored copies. */
    strcpy(kbuf, "zzzzz");
    strcpy(vbuf, "wwwww");
    ASSERT_STR_EQ(ht_get(ht, "mykey"), "myval");
    ASSERT_NULL(ht_get(ht, "zzzzz"));
    ht_destroy(ht);
}

static void test_grow_and_retrieve(void) {
    ht_t *ht = new_str_table();
    enum { N = 10000 };
    for (int i = 0; i < N; i++) {
        char k[16], v[16];
        snprintf(k, sizeof(k), "k%d", i);
        snprintf(v, sizeof(v), "v%d", i);
        ht_insert(ht, k, v);
    }
    ASSERT_EQ_UINT(ht_size(ht), N);
    for (int i = 0; i < N; i++) {
        char k[16], v[16];
        snprintf(k, sizeof(k), "k%d", i);
        snprintf(v, sizeof(v), "v%d", i);
        ASSERT_STR_EQ(ht_get(ht, k), v);
    }
    /* Remove evens; odds remain. */
    for (int i = 0; i < N; i += 2) {
        char k[16];
        snprintf(k, sizeof(k), "k%d", i);
        ht_remove(ht, k);
    }
    ASSERT_EQ_UINT(ht_size(ht), N / 2);
    for (int i = 0; i < N; i++) {
        char k[16];
        snprintf(k, sizeof(k), "k%d", i);
        if (i % 2 == 0) {
            ASSERT_NULL(ht_get(ht, k));
        } else {
            ASSERT_NOT_NULL(ht_get(ht, k));
        }
    }
    ht_destroy(ht);
}

static void test_collision_chain_operations(void) {
    ht_t *ht = ht_create(&(ht_options_t){.hash = all_collide,
                                         .keyeq = str_eq,
                                         .keylen = str_len,
                                         .callbacks = STR_CB});
    ASSERT_NOT_NULL(ht);
    const char *keys[] = {"a", "b", "c", "d", "e"};
    for (size_t i = 0; i < 5; i++) {
        ht_insert(ht, keys[i], keys[i]);
    }
    ASSERT_EQ_UINT(ht_size(ht), 5);
    for (size_t i = 0; i < 5; i++) {
        ASSERT_STR_EQ(ht_get(ht, keys[i]), keys[i]);
    }
    /* Remove head, an interior node, and the tail of the single chain. */
    ht_remove(ht, "a");
    ht_remove(ht, "c");
    ht_remove(ht, "e");
    ASSERT_EQ_UINT(ht_size(ht), 2);
    ASSERT_NULL(ht_get(ht, "a"));
    ASSERT_NULL(ht_get(ht, "c"));
    ASSERT_NULL(ht_get(ht, "e"));
    ASSERT_STR_EQ(ht_get(ht, "b"), "b");
    ASSERT_STR_EQ(ht_get(ht, "d"), "d");
    ht_destroy(ht);
}

static void test_null_args_are_safe(void) {
    ASSERT_NULL(ht_create(NULL));
    ASSERT_NULL(ht_get(NULL, "k"));
    ASSERT_EQ_UINT(ht_size(NULL), 0);
    ASSERT_FALSE(ht_contains(NULL, "k"));
    ht_insert(NULL, "k", "v"); /* no crash */
    ht_remove(NULL, "k");      /* no crash */
    ht_clear(NULL);            /* no crash */
    ht_destroy(NULL);          /* no crash */

    ht_t *ht = new_str_table();
    ht_insert(ht, NULL, "v"); /* NULL key ignored */
    ASSERT_EQ_UINT(ht_size(ht), 0);
    ht_destroy(ht);
}

/* ---- keyed tables ---- */

static void test_keyed_random_siphash(void) {
    ht_t *ht = ht_create(&(ht_options_t){.hash = ht_hash_siphash,
                                         .keyeq = str_eq,
                                         .keylen = str_len,
                                         .callbacks = STR_CB,
                                         .key_mode = HT_KEY_RANDOM});
    ASSERT_NOT_NULL(ht);
    ht_insert(ht, "alpha", "1");
    ht_insert(ht, "beta", "2");
    ht_insert(ht, "gamma", "3");
    ASSERT_STR_EQ(ht_get(ht, "beta"), "2");
    ht_remove(ht, "beta");
    ASSERT_NULL(ht_get(ht, "beta"));
    ASSERT_EQ_UINT(ht_size(ht), 2);
    ht_destroy(ht);
}

static void test_keyed_provided(void) {
    unsigned char key[16];
    for (int i = 0; i < 16; i++) {
        key[i] = (unsigned char)(i * 7 + 1);
    }
    ht_t *ht = ht_create(&(ht_options_t){.hash = ht_hash_siphash,
                                         .keyeq = str_eq,
                                         .keylen = str_len,
                                         .callbacks = STR_CB,
                                         .key_mode = HT_KEY_PROVIDED,
                                         .key = key});
    ASSERT_NOT_NULL(ht);
    ht_insert(ht, "one", "1");
    ASSERT_STR_EQ(ht_get(ht, "one"), "1");
    ht_destroy(ht);

    /* HT_KEY_PROVIDED without key material must fail. */
    ASSERT_NULL(ht_create(&(ht_options_t){.hash = ht_hash_siphash,
                                          .keyeq = str_eq,
                                          .keylen = str_len,
                                          .callbacks = STR_CB,
                                          .key_mode = HT_KEY_PROVIDED}));
}

/* ---- user_data ---- */

typedef struct {
    const void *seen;
    size_t copies;
    size_t frees;
} ud_ctx_t;

static void *ud_copy(const void *s, void *user_data) {
    ud_ctx_t *c = user_data;
    c->seen = user_data;
    c->copies++;
    return str_copy(s, NULL);
}

static void ud_free(const void *s, void *user_data) {
    ud_ctx_t *c = user_data;
    c->seen = user_data;
    c->frees++;
    str_free(s, NULL);
}

static void test_user_data_forwarded(void) {
    ud_ctx_t ctx = {0};
    const ht_callbacks_t cb = {ud_copy, ud_free, ud_copy, ud_free};
    ht_t *ht = ht_create(&(ht_options_t){.hash = ht_hash_fnv1a,
                                         .keyeq = str_eq,
                                         .keylen = str_len,
                                         .callbacks = cb,
                                         .user_data = &ctx});
    ASSERT_NOT_NULL(ht);
    ht_insert(ht, "a", "1");
    ht_insert(ht, "b", "2");
    ASSERT_EQ_UINT(ctx.copies, 4);
    ASSERT_EQ_PTR(ctx.seen, &ctx);
    ht_insert(ht, "a", "uno"); /* frees old value once */
    ASSERT_EQ_UINT(ctx.frees, 1);
    ht_destroy(ht);
    ASSERT_EQ_UINT(ctx.frees, 5); /* 1 replace + 2 keys + 2 values */
    ASSERT_EQ_PTR(ctx.seen, &ctx);
}

/* ---- statistics ---- */

static void test_stats_empty_and_populated(void) {
    ht_t *ht = new_str_table();
    ht_stats_t s;
    ht_stats(ht, &s);
    ASSERT_EQ_UINT(s.size, 0);
    ASSERT_EQ_UINT(s.occupied_buckets, 0);
    ASSERT_EQ_UINT(s.max_chain, 0);
    ASSERT_EQ_UINT(s.grow_count, 0);
    ASSERT_TRUE(s.capacity > 0);
    ASSERT_EQ_DBL(s.load_factor, 0.0, 0.0);

    enum { N = 5000 };
    for (int i = 0; i < N; i++) {
        char k[16];
        snprintf(k, sizeof(k), "k%d", i);
        ht_insert(ht, k, "v");
    }
    ht_stats(ht, &s);
    ASSERT_EQ_UINT(s.size, N);
    ASSERT_TRUE(s.grow_count > 0);
    ASSERT_TRUE(s.occupied_buckets > 0);
    ASSERT_TRUE(s.occupied_buckets <= s.size);
    ASSERT_TRUE(s.max_chain >= 1 && s.max_chain <= s.size);
    ASSERT_EQ_DBL(s.load_factor, (double)s.size / (double)s.capacity, 1e-12);
    ASSERT_TRUE(s.load_factor < 0.75);

    ht_destroy(ht);
}

static void test_stats_null_guards(void) {
    ht_stats_t s;
    ht_stats(NULL, &s);
    ASSERT_EQ_UINT(s.size, 0);
    ASSERT_EQ_UINT(s.capacity, 0);
    ASSERT_EQ_DBL(s.load_factor, 0.0, 0.0);
    ht_stats(NULL, NULL); /* no crash */
}

/* ---- insertion order ---- */

static void collect_order(ht_t *ht, char out[][16], size_t *n) {
    ht_enum_t *he = ht_enum_create(ht);
    const void *k = NULL;
    size_t i = 0;
    while (ht_enum_next(he, &k, NULL)) {
        snprintf(out[i], 16, "%s", (const char *)k);
        i++;
    }
    ht_enum_destroy(he);
    *n = i;
}

static void test_ordered_through_rehash_and_replace(void) {
    ht_t *ht = ht_create(&(ht_options_t){.hash = ht_hash_fnv1a,
                                         .keyeq = str_eq,
                                         .keylen = str_len,
                                         .callbacks = STR_CB,
                                         .insertion_ordered = true});
    enum { N = 300 };
    for (int i = 0; i < N; i++) {
        char k[16];
        snprintf(k, sizeof(k), "k%d", i);
        ht_insert(ht, k, "v");
    }
    ht_insert(ht, "k150", "REPLACED"); /* keeps position */

    static char order[N][16];
    size_t got = 0;
    collect_order(ht, order, &got);
    ASSERT_EQ_UINT(got, N);
    for (int i = 0; i < N; i++) {
        char expect[16];
        snprintf(expect, sizeof(expect), "k%d", i);
        ASSERT_STR_EQ(order[i], expect);
    }
    ASSERT_STR_EQ(ht_get(ht, "k150"), "REPLACED");
    ht_destroy(ht);
}

static void test_ordered_remove_keeps_order(void) {
    ht_t *ht = ht_create(&(ht_options_t){.hash = all_collide,
                                         .keyeq = str_eq,
                                         .keylen = str_len,
                                         .callbacks = STR_CB,
                                         .insertion_ordered = true});
    const char *ins[] = {"a", "b", "c", "d", "e"};
    for (size_t i = 0; i < 5; i++) {
        ht_insert(ht, ins[i], ins[i]);
    }
    ht_remove(ht, "a"); /* head of chain */
    ht_remove(ht, "c"); /* interior */
    static char order[8][16];
    size_t got = 0;
    collect_order(ht, order, &got);
    ASSERT_EQ_UINT(got, 3);
    ASSERT_STR_EQ(order[0], "b");
    ASSERT_STR_EQ(order[1], "d");
    ASSERT_STR_EQ(order[2], "e");
    ht_destroy(ht);
}

/* ---- typed wrappers ---- */

static void test_wrapper_strstr_case(void) {
    ht_strstr_t *ht =
        ht_strstr_create(&(ht_str_options_t){.case_insensitive = true});
    ASSERT_NOT_NULL(ht);
    ht_strstr_insert(ht, "Hello", "world");
    ASSERT_STR_EQ(ht_strstr_get(ht, "HELLO"), "world");
    ASSERT_STR_EQ(ht_strstr_get(ht, "hello"), "world");
    ht_strstr_insert(ht, "hELLo", "earth"); /* same key, case-folded */
    ASSERT_STR_EQ(ht_strstr_get(ht, "Hello"), "earth");
    ht_strstr_destroy(ht);
}

static void test_wrapper_strint(void) {
    ht_strint_t *ht = ht_strint_create(NULL);
    ASSERT_NOT_NULL(ht);
    int a = 42, b = -7;
    ht_strint_insert(ht, "a", &a);
    ht_strint_insert(ht, "b", &b);
    /* Mutate the source; stored value is an independent copy. */
    a = 999;
    ASSERT_EQ_INT(*(int *)ht_strint_get(ht, "a"), 42);
    ASSERT_EQ_INT(*(int *)ht_strint_get(ht, "b"), -7);
    ASSERT_NULL(ht_strint_get(ht, "missing"));
    ht_strint_destroy(ht);
}

static void test_wrapper_strfloat_strdouble(void) {
    ht_strfloat_t *hf = ht_strfloat_create(NULL);
    float f = 3.5f;
    ht_strfloat_insert(hf, "pi", &f);
    ASSERT_EQ_DBL((double)*(float *)ht_strfloat_get(hf, "pi"), 3.5, 1e-6);
    ht_strfloat_destroy(hf);

    ht_strdouble_t *hd = ht_strdouble_create(NULL);
    double d = 2.718281828;
    ht_strdouble_insert(hd, "e", &d);
    ASSERT_EQ_DBL(*(double *)ht_strdouble_get(hd, "e"), 2.718281828, 1e-12);
    ht_strdouble_destroy(hd);
}

static void test_wrapper_strblob_embedded_nul(void) {
    ht_strblob_t *ht = ht_strblob_create(NULL);
    ASSERT_NOT_NULL(ht);
    unsigned char data[] = {0x00, 0x01, 0x00, 0xFF, 0x42, 0x00};
    ht_strblob_insert(ht, "bin", data, sizeof(data));

    /* Mutate the source; the stored blob is an independent deep copy. */
    data[0] = 0xAB;

    size_t out = 0;
    const void *got = ht_strblob_get(ht, "bin", &out);
    ASSERT_NOT_NULL(got);
    ASSERT_EQ_UINT(out, sizeof(data));
    const unsigned char expect[] = {0x00, 0x01, 0x00, 0xFF, 0x42, 0x00};
    ASSERT_MEM_EQ(got, expect, sizeof(expect));

    /* Absent key reports zero size. */
    size_t miss = 123;
    ASSERT_NULL(ht_strblob_get(ht, "nope", &miss));
    ASSERT_EQ_UINT(miss, 0);
    ht_strblob_destroy(ht);
}

static void test_wrapper_strptr_identity(void) {
    ht_strptr_t *ht = ht_strptr_create(NULL);
    int obj = 1;
    ht_strptr_insert(ht, "p", &obj);
    /* The exact pointer is stored, not a copy. */
    ASSERT_EQ_PTR(ht_strptr_get(ht, "p"), &obj);
    ht_strptr_remove(ht, "p");
    ASSERT_NULL(ht_strptr_get(ht, "p"));
    ht_strptr_destroy(ht);
}

static void test_wrapper_strset(void) {
    ht_strset_t *ht = ht_strset_create(NULL);
    ASSERT_TRUE(ht_strset_add(ht, "x"));  /* newly added */
    ASSERT_FALSE(ht_strset_add(ht, "x")); /* already present */
    ASSERT_TRUE(ht_strset_contains(ht, "x"));
    ASSERT_FALSE(ht_strset_contains(ht, "y"));
    ht_strset_remove(ht, "x");
    ASSERT_FALSE(ht_strset_contains(ht, "x"));
    ht_strset_destroy(ht);
}

static void test_wrapper_u64ptr_highbits(void) {
    ht_u64ptr_t *ht = ht_u64ptr_create(NULL);
    int objs[3] = {10, 20, 30};
    ht_u64ptr_insert(ht, 1, &objs[0]);
    ht_u64ptr_insert(ht, 0x100000000ULL, &objs[1]); /* differs only high 32 */
    ht_u64ptr_insert(ht, 0, &objs[2]);
    ASSERT_EQ_PTR(ht_u64ptr_get(ht, 1), &objs[0]);
    ASSERT_EQ_PTR(ht_u64ptr_get(ht, 0x100000000ULL), &objs[1]);
    ASSERT_EQ_PTR(ht_u64ptr_get(ht, 0), &objs[2]);
    ASSERT_TRUE(ht_u64ptr_get(ht, 0x100000000ULL) != ht_u64ptr_get(ht, 0));
    ASSERT_NULL(ht_u64ptr_get(ht, 99));
    ht_u64ptr_destroy(ht);
}

static void test_wrapper_u64blob(void) {
    ht_u64blob_t *ht = ht_u64blob_create(NULL);
    unsigned char a[] = {1, 2, 3};
    unsigned char b[] = {9, 8, 7, 6};
    ht_u64blob_insert(ht, 100, a, sizeof(a));
    ht_u64blob_insert(ht, 200, b, sizeof(b));
    size_t n = 0;
    const void *g = ht_u64blob_get(ht, 200, &n);
    ASSERT_EQ_UINT(n, 4);
    ASSERT_MEM_EQ(g, b, 4);
    ht_u64blob_remove(ht, 100);
    ASSERT_NULL(ht_u64blob_get(ht, 100, &n));
    ASSERT_EQ_UINT(n, 0);
    ht_u64blob_destroy(ht);
}

static void test_wrapper_ordered(void) {
    ht_strstr_t *ht =
        ht_strstr_create(&(ht_str_options_t){.insertion_ordered = true});
    ht_strstr_insert(ht, "first", "1");
    ht_strstr_insert(ht, "second", "2");
    ht_strstr_insert(ht, "third", "3");
    ht_strstr_remove(ht, "second");
    ht_strstr_insert(ht, "fourth", "4");

    const char *want[] = {"first", "third", "fourth"};
    ht_enum_t *he = ht_strstr_enum_create(ht);
    const char *k = NULL, *v = NULL;
    size_t i = 0;
    while (ht_strstr_enum_next(he, &k, &v)) {
        ASSERT_TRUE(i < 3);
        ASSERT_STR_EQ(k, want[i]);
        i++;
    }
    ht_strstr_enum_destroy(he);
    ASSERT_EQ_UINT(i, 3);
    ht_strstr_destroy(ht);
}

/* ---- hash-algorithm contracts ---- */

static void test_hash_fnv1a_contract(void) {
    ht_hash_t h1 = ht_hash_fnv1a("abc", 3, NULL);
    ht_hash_t h2 = ht_hash_fnv1a("abc", 3, NULL);
    ht_hash_t h3 = ht_hash_fnv1a("abd", 3, NULL);
    ASSERT_EQ_UINT(h1, h2); /* deterministic */
    ASSERT_TRUE(h1 != h3);  /* sensitive to input */
}

static void test_hash_int_distinct(void) {
    /* The integer finalizer is a bijection on 64-bit values: distinct keys
       must map to distinct hashes. */
    uint64_t seed = 0xABCDEF;
    uint64_t prev[256];
    for (int i = 0; i < 256; i++) {
        uint64_t k = splitmix64(&seed);
        prev[i] = ht_hash_int(&k, sizeof(k), NULL);
        for (int j = 0; j < i; j++) {
            ASSERT_TRUE(prev[i] != prev[j]);
        }
    }
}

static void test_hash_siphash_keyed_property(void) {
    unsigned char k1[16], k2[16];
    for (int i = 0; i < 16; i++) {
        k1[i] = (unsigned char)i;
        k2[i] = (unsigned char)(i + 1);
    }
    ht_hash_t a = ht_hash_siphash("message", 7, k1);
    ht_hash_t a2 = ht_hash_siphash("message", 7, k1);
    ht_hash_t b = ht_hash_siphash("message", 7, k2);
    ASSERT_EQ_UINT(a, a2); /* deterministic under a fixed key */
    ASSERT_TRUE(a != b);   /* different key, different hash */
}

int main(void) {
    ht_test_begin("functional");

    RUN_TEST(test_insert_get);
    RUN_TEST(test_get_absent);
    RUN_TEST(test_contains_distinguishes_null_value);
    RUN_TEST(test_replace_value);
    RUN_TEST(test_size_tracking);
    RUN_TEST(test_remove);
    RUN_TEST(test_upsert);
    RUN_TEST(test_get_or_insert);
    RUN_TEST(test_clear_then_reuse);
    RUN_TEST(test_foreach_visits_each_once);
    RUN_TEST(test_enum_visits_each_once);
    RUN_TEST(test_deep_copy_independent_of_caller_buffers);
    RUN_TEST(test_grow_and_retrieve);
    RUN_TEST(test_collision_chain_operations);
    RUN_TEST(test_null_args_are_safe);

    RUN_TEST(test_keyed_random_siphash);
    RUN_TEST(test_keyed_provided);

    RUN_TEST(test_user_data_forwarded);

    RUN_TEST(test_stats_empty_and_populated);
    RUN_TEST(test_stats_null_guards);

    RUN_TEST(test_ordered_through_rehash_and_replace);
    RUN_TEST(test_ordered_remove_keeps_order);

    RUN_TEST(test_wrapper_strstr_case);
    RUN_TEST(test_wrapper_strint);
    RUN_TEST(test_wrapper_strfloat_strdouble);
    RUN_TEST(test_wrapper_strblob_embedded_nul);
    RUN_TEST(test_wrapper_strptr_identity);
    RUN_TEST(test_wrapper_strset);
    RUN_TEST(test_wrapper_u64ptr_highbits);
    RUN_TEST(test_wrapper_u64blob);
    RUN_TEST(test_wrapper_ordered);

    RUN_TEST(test_hash_fnv1a_contract);
    RUN_TEST(test_hash_int_distinct);
    RUN_TEST(test_hash_siphash_keyed_property);

    return ht_test_summary();
}

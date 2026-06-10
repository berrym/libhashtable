/**
 * @file ht_integration_test.c
 * @brief Integration tests against real data and an independent oracle.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 *
 * Drives the table with real input (the project's own source files as a text
 * corpus) and cross-checks every result against a deliberately simple, fully
 * independent reference: a linear-scan association list that shares no code
 * with the hash table. A word-frequency table, an insertion-ordered first-seen
 * set, a large unique-key stress, and a binary-blob round trip each confirm the
 * table agrees with the oracle, loses nothing, and copies what it stores.
 */

#include "ht.h"
#include "ht_test.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HT_SRCROOT
#define HT_SRCROOT "."
#endif

/// Real input files; all are committed to the repository.
static const char *const CORPUS_FILES[] = {
    HT_SRCROOT "/src/ht.c",         HT_SRCROOT "/include/ht.h",
    HT_SRCROOT "/src/ht_strstr.c",  HT_SRCROOT "/src/ht_fnv1a.c",
    HT_SRCROOT "/src/ht_siphash.c",
};

/* ---- independent reference oracle: a linear-scan association list ---- */

typedef struct {
    char *word;
    size_t count;
    size_t first_seen; /* 0-based order of first appearance */
} ref_entry_t;

typedef struct {
    ref_entry_t *items;
    size_t len;
    size_t cap;
} ref_t;

static void ref_bump(ref_t *r, const char *word) {
    for (size_t i = 0; i < r->len; i++) {
        if (strcmp(r->items[i].word, word) == 0) {
            r->items[i].count++;
            return;
        }
    }
    if (r->len == r->cap) {
        r->cap = r->cap ? r->cap * 2 : 64;
        r->items = realloc(r->items, r->cap * sizeof(*r->items));
        if (!r->items) {
            abort();
        }
    }
    r->items[r->len].word = strdup(word);
    r->items[r->len].count = 1;
    r->items[r->len].first_seen = r->len;
    r->len++;
}

static const ref_entry_t *ref_find(const ref_t *r, const char *word) {
    for (size_t i = 0; i < r->len; i++) {
        if (strcmp(r->items[i].word, word) == 0) {
            return &r->items[i];
        }
    }
    return NULL;
}

static void ref_free(ref_t *r) {
    for (size_t i = 0; i < r->len; i++) {
        free(r->items[i].word);
    }
    free(r->items);
    r->items = NULL;
    r->len = r->cap = 0;
}

/* ---- corpus loading and tokenization ---- */

static char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[n] = '\0';
    *out_len = n;
    return buf;
}

/// Lowercase a-z token boundary scan. Calls sink() for each token.
static void tokenize(const char *buf, size_t len,
                     void (*sink)(const char *tok, void *ctx), void *ctx) {
    char tok[64];
    size_t tl = 0;
    for (size_t i = 0; i <= len; i++) {
        int c = (i < len) ? (unsigned char)buf[i] : 0;
        if (c >= 'A' && c <= 'Z') {
            c = c - 'A' + 'a';
        }
        if (c >= 'a' && c <= 'z') {
            if (tl + 1 < sizeof(tok)) {
                tok[tl++] = (char)c;
            }
        } else if (tl > 0) {
            tok[tl] = '\0';
            sink(tok, ctx);
            tl = 0;
        }
    }
}

/* ---- the structures fed from one identical token stream ---- */

typedef struct {
    ref_t ref;              /* the independent oracle */
    ht_strint_t *counts;    /* word -> frequency, the system under test */
    ht_strset_t *firstseen; /* insertion-ordered presence set */
    size_t total_tokens;
} corpus_t;

static void corpus_sink(const char *tok, void *ctx) {
    corpus_t *c = ctx;
    c->total_tokens++;
    ref_bump(&c->ref, tok);

    const int *cur = ht_strint_get(c->counts, tok);
    int next = cur ? *cur + 1 : 1;
    ht_strint_insert(c->counts, tok, &next);

    ht_strset_add(c->firstseen, tok);
}

/* ---- tests ---- */

/// Every word frequency, the distinct-word count, and the total agree with the
/// oracle; enumeration yields each word exactly once; removal drains the table.
static void test_wordcount_matches_oracle(void) {
    corpus_t c = {0};
    c.counts = ht_strint_create(NULL);
    c.firstseen =
        ht_strset_create(&(ht_str_options_t){.insertion_ordered = true});
    ASSERT_NOT_NULL(c.counts);
    ASSERT_NOT_NULL(c.firstseen);

    size_t files = sizeof(CORPUS_FILES) / sizeof(CORPUS_FILES[0]);
    for (size_t i = 0; i < files; i++) {
        size_t len = 0;
        char *buf = read_file(CORPUS_FILES[i], &len);
        if (!buf) {
            fprintf(stderr, "  cannot open corpus file %s\n", CORPUS_FILES[i]);
            FAIL_MSG("corpus file missing; pass -DHT_SRCROOT");
        }
        tokenize(buf, len, corpus_sink, &c);
        free(buf);
    }

    /* The corpus must be non-trivial for the test to mean anything. */
    ASSERT_TRUE(c.ref.len > 100);
    ASSERT_TRUE(c.total_tokens > c.ref.len);

    /* Distinct-word count agrees. */
    ASSERT_EQ_UINT(ht_size((const ht_t *)c.counts), c.ref.len);

    /* Every oracle frequency is reproduced, and the totals reconcile. */
    size_t sum = 0;
    for (size_t i = 0; i < c.ref.len; i++) {
        const int *got = ht_strint_get(c.counts, c.ref.items[i].word);
        ASSERT_NOT_NULL(got);
        ASSERT_EQ_UINT((size_t)*got, c.ref.items[i].count);
        sum += c.ref.items[i].count;
    }
    ASSERT_EQ_UINT(sum, c.total_tokens);

    /* Enumeration visits every distinct word exactly once with a value that
       matches the oracle. */
    int *visits = calloc(c.ref.len, sizeof(int));
    ASSERT_NOT_NULL(visits);
    ht_enum_t *he = ht_strint_enum_create(c.counts);
    const char *k = NULL;
    const int *v = NULL;
    size_t enumerated = 0;
    while (ht_strint_enum_next(he, &k, &v)) {
        const ref_entry_t *e = ref_find(&c.ref, k);
        ASSERT_NOT_NULL(e);
        ASSERT_EQ_UINT((size_t)*v, e->count);
        visits[e->first_seen]++;
        enumerated++;
    }
    ht_strint_enum_destroy(he);
    ASSERT_EQ_UINT(enumerated, c.ref.len);
    for (size_t i = 0; i < c.ref.len; i++) {
        ASSERT_EQ_INT(visits[i], 1);
    }
    free(visits);

    /* Removing every oracle word drains the table to empty. */
    for (size_t i = 0; i < c.ref.len; i++) {
        ht_strint_remove(c.counts, c.ref.items[i].word);
    }
    ASSERT_EQ_UINT(ht_size((const ht_t *)c.counts), 0);

    ht_strint_destroy(c.counts);
    ht_strset_destroy(c.firstseen);
    ref_free(&c.ref);
}

/// The insertion-ordered set enumerates words in first-seen order, exactly as
/// the oracle recorded them.
static void test_first_seen_order_matches_oracle(void) {
    corpus_t c = {0};
    c.counts = ht_strint_create(NULL);
    c.firstseen =
        ht_strset_create(&(ht_str_options_t){.insertion_ordered = true});

    size_t files = sizeof(CORPUS_FILES) / sizeof(CORPUS_FILES[0]);
    for (size_t i = 0; i < files; i++) {
        size_t len = 0;
        char *buf = read_file(CORPUS_FILES[i], &len);
        if (!buf) {
            FAIL_MSG("corpus file missing");
        }
        tokenize(buf, len, corpus_sink, &c);
        free(buf);
    }

    ht_enum_t *he = ht_strset_enum_create(c.firstseen);
    const char *k = NULL;
    size_t i = 0;
    while (ht_strset_enum_next(he, &k)) {
        ASSERT_TRUE(i < c.ref.len);
        ASSERT_STR_EQ(k,
                      c.ref.items[i].word); /* oracle is in first-seen order */
        i++;
    }
    ht_strset_enum_destroy(he);
    ASSERT_EQ_UINT(i, c.ref.len);

    ht_strint_destroy(c.counts);
    ht_strset_destroy(c.firstseen);
    ref_free(&c.ref);
}

/// A large set of unique 64-bit keys is stored and retrieved with no loss, then
/// fully removed; values alias caller objects exactly.
static void test_large_uint64_no_loss(void) {
    enum { N = 200000 };
    uint64_t *keys = malloc(N * sizeof(*keys));
    int *objs = malloc(N * sizeof(*objs));
    ASSERT_NOT_NULL(keys);
    ASSERT_NOT_NULL(objs);

    /* splitmix64 is a bijection, so distinct inputs give distinct keys. */
    uint64_t s = 0x123456789ULL;
    for (int i = 0; i < N; i++) {
        s += 0x9E3779B97F4A7C15ULL;
        uint64_t z = s;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        keys[i] = z ^ (z >> 31);
        objs[i] = i;
    }

    ht_u64ptr_t *ht = ht_u64ptr_create(NULL);
    ASSERT_NOT_NULL(ht);
    for (int i = 0; i < N; i++) {
        ht_u64ptr_insert(ht, keys[i], &objs[i]);
    }
    ASSERT_EQ_UINT(ht_size((const ht_t *)ht), N);

    for (int i = 0; i < N; i++) {
        ASSERT_EQ_PTR(ht_u64ptr_get(ht, keys[i]), &objs[i]);
    }

    /* Remove in a different (strided) order; every entry must clear. */
    size_t removed = 0;
    for (int stride = 0; stride < 7; stride++) {
        for (int i = stride; i < N; i += 7) {
            ht_u64ptr_remove(ht, keys[i]);
            removed++;
        }
    }
    ASSERT_EQ_UINT(removed, N);
    ASSERT_EQ_UINT(ht_size((const ht_t *)ht), 0);

    ht_u64ptr_destroy(ht);
    free(keys);
    free(objs);
}

/// Binary blobs with embedded NULs round-trip byte-for-byte through the table
/// and survive mutation of the caller's source buffer.
static void test_binary_blob_roundtrip(void) {
    enum { N = 2000 };
    ht_u64blob_t *ht = ht_u64blob_create(NULL);
    ASSERT_NOT_NULL(ht);

    uint64_t s = 0xFEEDFACEULL;
    for (int i = 0; i < N; i++) {
        unsigned char payload[32];
        s += 0x9E3779B97F4A7C15ULL;
        for (size_t b = 0; b < sizeof(payload); b++) {
            s = s * 6364136223846793005ULL + 1442695040888963407ULL;
            payload[b] = (unsigned char)(s >> 56);
        }
        ht_u64blob_insert(ht, (uint64_t)i, payload, sizeof(payload));
        /* Scribble the source; the stored copy must be unaffected. */
        memset(payload, 0xCC, sizeof(payload));
    }
    ASSERT_EQ_UINT(ht_size((const ht_t *)ht), N);

    /* Recompute each payload deterministically and compare to what is stored.
     */
    s = 0xFEEDFACEULL;
    for (int i = 0; i < N; i++) {
        unsigned char expect[32];
        s += 0x9E3779B97F4A7C15ULL;
        for (size_t b = 0; b < sizeof(expect); b++) {
            s = s * 6364136223846793005ULL + 1442695040888963407ULL;
            expect[b] = (unsigned char)(s >> 56);
        }
        size_t got_len = 0;
        const void *got = ht_u64blob_get(ht, (uint64_t)i, &got_len);
        ASSERT_NOT_NULL(got);
        ASSERT_EQ_UINT(got_len, sizeof(expect));
        ASSERT_MEM_EQ(got, expect, sizeof(expect));
    }

    ht_u64blob_destroy(ht);
}

int main(void) {
    ht_test_begin("integration");
    RUN_TEST(test_wordcount_matches_oracle);
    RUN_TEST(test_first_seen_order_matches_oracle);
    RUN_TEST(test_large_uint64_no_loss);
    RUN_TEST(test_binary_blob_roundtrip);
    return ht_test_summary();
}

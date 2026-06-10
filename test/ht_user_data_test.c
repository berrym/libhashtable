/**
 * @file ht_user_data_test.c
 * @brief Test program for per-table user_data forwarded to callbacks
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 */

#include "ht.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Context handed to every callback through the table's user_data pointer. It
/// records how many times each callback ran and the user_data each one saw, so
/// the test can confirm the table forwards the exact pointer it was given.
typedef struct {
    const void *seen; ///< user_data observed by the most recent callback
    size_t copies;    ///< number of copy-callback invocations
    size_t frees;     ///< number of free-callback invocations
} cb_ctx_t;

/// Duplicate a string key/value while recording the user_data it received.
static void *ctx_copy(const void *s, void *user_data) {
    cb_ctx_t *ctx = user_data;
    ctx->seen = user_data;
    ctx->copies++;
    return strdup(s);
}

/// Free a string key/value while recording the user_data it received.
static void ctx_free(const void *s, void *user_data) {
    cb_ctx_t *ctx = user_data;
    ctx->seen = user_data;
    ctx->frees++;
    free((void *)s);
}

int main(void) {
    cb_ctx_t ctx = {0};

    const ht_callbacks_t cb = {ctx_copy, ctx_free, ctx_copy, ctx_free};
    ht_t *ht = ht_create(&(ht_options_t){.hash = ht_hash_fnv1a,
                                         .keyeq = str_eq,
                                         .keylen = str_len,
                                         .callbacks = cb,
                                         .user_data = &ctx});
    if (!ht) {
        fprintf(stderr, "ht_create failed\n");
        exit(EXIT_FAILURE);
    }

    /// Each insert copies a key and a value, so the callbacks must have run and
    /// must have seen the context pointer passed through user_data.
    ht_insert(ht, "alpha", "one");
    ht_insert(ht, "beta", "two");
    if (ctx.copies != 4) {
        fprintf(stderr, "copy callbacks ran %zu times, expected 4\n",
                ctx.copies);
        exit(EXIT_FAILURE);
    }
    if (ctx.seen != &ctx) {
        fprintf(stderr, "callback did not receive the table's user_data\n");
        exit(EXIT_FAILURE);
    }

    /// Replacing a value frees the old one and copies the new one, all while
    /// forwarding the same user_data.
    ht_insert(ht, "alpha", "uno");
    if (ctx.frees != 1) {
        fprintf(stderr, "value replacement freed %zu times, expected 1\n",
                ctx.frees);
        exit(EXIT_FAILURE);
    }

    /// Destroying the table frees every remaining key and value through the
    /// same forwarded user_data.
    ht_destroy(ht);
    if (ctx.frees != 1 + 4) {
        fprintf(stderr, "total frees %zu, expected 5\n", ctx.frees);
        exit(EXIT_FAILURE);
    }
    if (ctx.seen != &ctx) {
        fprintf(stderr,
                "free callback did not receive the table's user_data\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

/**
 * @file ht_u64blob.c
 * @brief Type-wrapped uint64->binary-blob hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 *
 * Maps a 64-bit integer key to an arbitrary binary value (including embedded
 * NUL bytes) with explicit length tracking. The key is hashed directly with
 * the integer finalizer; both the key and the blob bytes are deep-copied.
 */

#include "ht.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Copy a uint64 key onto the heap (key-copy callback). The user_data argument
/// is unused.
static void *u64_key_copy(const void *key, void *user_data) {
    (void)user_data;
    uint64_t *k = malloc(sizeof(*k));
    if (!k) {
        perror("ht_u64blob_insert");
        return NULL;
    }
    *k = *(const uint64_t *)key;
    return k;
}

/// Free a heap-copied uint64 key (key-free callback). The user_data argument
/// is unused.
static void u64_key_free(const void *key, void *user_data) {
    (void)user_data;
    free((void *)key);
}

/// Compare two uint64 keys (key-equality callback).
static bool u64_key_eq(const void *a, const void *b) {
    return *(const uint64_t *)a == *(const uint64_t *)b;
}

/// Report the key length (key-length callback).
static size_t u64_key_len(const void *key) {
    (void)key;
    return sizeof(uint64_t);
}

/// Internal value type: owns a heap copy of the caller's bytes plus the length.
typedef struct {
    void *data;
    size_t size;
} ht_blob_t;

/// Deep-copy a blob (value-copy callback). The user_data argument is unused.
static void *blob_copy(const void *src, void *user_data) {
    (void)user_data;
    const ht_blob_t *s = src;
    ht_blob_t *d = malloc(sizeof(*d));
    if (!d) {
        perror("ht_u64blob_insert");
        return NULL;
    }

    d->size = s->size;
    if (s->size == 0) {
        d->data = NULL;
        return d;
    }

    d->data = malloc(s->size);
    if (!d->data) {
        perror("ht_u64blob_insert");
        free(d);
        return NULL;
    }
    memcpy(d->data, s->data, s->size);
    return d;
}

/// Free a blob and its owned bytes (value-free callback). The user_data
/// argument is unused.
static void blob_free(const void *blob, void *user_data) {
    (void)user_data;
    if (!blob) {
        return;
    }
    const ht_blob_t *b = blob;
    free(b->data);
    free((void *)b);
}

/// Wrapper around ht_create that creates a uint64->blob hash table.
ht_u64blob_t *ht_u64blob_create(const ht_u64_options_t *opts) {
    const ht_u64_options_t o = opts ? *opts : (ht_u64_options_t){0};

    const ht_callbacks_t callbacks = {u64_key_copy, u64_key_free, blob_copy,
                                      blob_free};

    const ht_options_t base = {
        .hash = o.flooding_resistant ? ht_hash_siphash : ht_hash_int,
        .keyeq = u64_key_eq,
        .keylen = u64_key_len,
        .callbacks = callbacks,
        .key_mode = o.flooding_resistant ? HT_KEY_RANDOM : HT_KEY_NONE,
        .key_best_effort = o.best_effort,
        .insertion_ordered = o.insertion_ordered,
        .initial_capacity = o.initial_capacity,
    };

    return (ht_u64blob_t *)ht_create(&base);
}

/// Wrapper around ht_destroy for a uint64->blob hash table.
void ht_u64blob_destroy(ht_u64blob_t *ht) { ht_destroy((ht_t *)ht); }

/// Insert a uint64 key mapping to a deep copy of size bytes at data. The
/// caller retains ownership of its input buffer; size may be 0.
void ht_u64blob_insert(ht_u64blob_t *ht, uint64_t key, const void *data,
                       size_t size) {
    const ht_blob_t blob = {(void *)data, size};
    ht_insert((ht_t *)ht, &key, &blob);
}

/// Remove the entry for a uint64 key.
void ht_u64blob_remove(ht_u64blob_t *ht, uint64_t key) {
    ht_remove((ht_t *)ht, &key);
}

/// Look up the blob stored under a uint64 key. The returned pointer aliases
/// the table's storage and must not be freed. out_size, if non-NULL, receives
/// the blob length, or 0 when the key is absent.
const void *ht_u64blob_get(ht_u64blob_t *ht, uint64_t key, size_t *out_size) {
    const ht_blob_t *blob = ht_get((ht_t *)ht, &key);
    if (!blob) {
        if (out_size) {
            *out_size = 0;
        }
        return NULL;
    }
    if (out_size) {
        *out_size = blob->size;
    }
    return blob->data;
}

/// Wrapper around ht_enum_create for a uint64->blob hash table.
ht_enum_t *ht_u64blob_enum_create(ht_u64blob_t *ht) {
    return ht_enum_create((ht_t *)ht);
}

/// Return the next key/blob pair. The data pointer aliases the table's
/// storage and must not be freed.
bool ht_u64blob_enum_next(ht_enum_t *he, uint64_t *key, const void **data,
                          size_t *size) {
    const void *kp = NULL;
    const void *vp = NULL;
    if (!ht_enum_next(he, &kp, &vp)) {
        return false;
    }

    const ht_blob_t *blob = vp;
    if (key) {
        *key = *(const uint64_t *)kp;
    }
    if (data) {
        *data = blob ? blob->data : NULL;
    }
    if (size) {
        *size = blob ? blob->size : 0;
    }
    return true;
}

/// Wrapper around ht_enum_destroy for a uint64->blob enumeration object.
void ht_u64blob_enum_destroy(ht_enum_t *he) { ht_enum_destroy(he); }

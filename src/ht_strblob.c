/**
 * @file ht_strblob.c
 * @brief Type-wrapped string->binary-blob hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 *
 * Stores arbitrary binary data, including embedded NUL bytes, under a string
 * key with explicit length tracking. Use this in place of ht_strstr when a
 * value can contain bytes that are unsafe for C-string semantics (serialized
 * structures, terminal control bytes, image data, and so on).
 */

#include "ht.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Internal value type: owns a heap copy of the caller's bytes plus the
/// length. Callers interact only through the (data, size) API.
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
        perror("ht_strblob_insert");
        return NULL;
    }

    d->size = s->size;
    if (s->size == 0) {
        d->data = NULL;
        return d;
    }

    d->data = malloc(s->size);
    if (!d->data) {
        perror("ht_strblob_insert");
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

/// Wrapper around ht_create that creates a string->blob hash table.
ht_strblob_t *ht_strblob_create(const ht_str_options_t *opts) {
    const ht_str_options_t o = opts ? *opts : (ht_str_options_t){0};
    if (o.case_insensitive && o.flooding_resistant) {
        return NULL;
    }

    const ht_callbacks_t callbacks = {str_copy, str_free, blob_copy, blob_free};

    const ht_options_t base = {
        .hash = o.flooding_resistant ? ht_hash_siphash
                : o.case_insensitive ? ht_hash_fnv1a_casecmp
                                     : ht_hash_fnv1a,
        .keyeq = o.case_insensitive ? str_caseeq : str_eq,
        .keylen = str_len,
        .callbacks = callbacks,
        .key_mode = o.flooding_resistant ? HT_KEY_RANDOM : HT_KEY_NONE,
        .key_best_effort = o.best_effort,
        .insertion_ordered = o.insertion_ordered,
        .initial_capacity = o.initial_capacity,
    };

    return (ht_strblob_t *)ht_create(&base);
}

/// Wrapper around ht_destroy that destroys a string->blob hash table.
void ht_strblob_destroy(ht_strblob_t *ht) { ht_destroy((ht_t *)ht); }

/// Insert a key/blob pair. The table deep-copies both the key and the blob
/// bytes; the caller retains ownership of its input buffer. size may be 0.
void ht_strblob_insert(ht_strblob_t *ht, const char *key, const void *data,
                       size_t size) {
    const ht_blob_t blob = {(void *)data, size};
    ht_insert((ht_t *)ht, (void *)key, &blob);
}

/// Wrapper around ht_remove for a string->blob hash table.
void ht_strblob_remove(ht_strblob_t *ht, const char *key) {
    ht_remove((ht_t *)ht, (void *)key);
}

/// Look up the blob stored under a key. The returned pointer aliases the
/// table's storage and must not be freed. out_size, if non-NULL, receives the
/// blob length, or 0 when the key is absent.
const void *ht_strblob_get(ht_strblob_t *ht, const char *key,
                           size_t *out_size) {
    const ht_blob_t *blob = ht_get((ht_t *)ht, (void *)key);
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

/// Wrapper around ht_enum_create for a string->blob hash table.
ht_enum_t *ht_strblob_enum_create(ht_strblob_t *ht) {
    return ht_enum_create((ht_t *)ht);
}

/// Return the next key/blob pair. The pointers written through key, data,
/// and size alias the table's storage and must not be freed.
bool ht_strblob_enum_next(ht_enum_t *he, const char **key, const void **data,
                          size_t *size) {
    const void *raw_val = NULL;
    if (!ht_enum_next(he, (const void **)key, &raw_val)) {
        return false;
    }

    const ht_blob_t *blob = raw_val;
    if (data) {
        *data = blob ? blob->data : NULL;
    }
    if (size) {
        *size = blob ? blob->size : 0;
    }
    return true;
}

/// Wrapper around ht_enum_destroy for a string->blob enumeration object.
void ht_strblob_enum_destroy(ht_enum_t *he) { ht_enum_destroy(he); }

/**
 * @file ht_u64ptr.c
 * @brief Type-wrapped uint64->pointer hash table.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 *
 * Maps a 64-bit integer key to an opaque, caller-owned pointer value. The key
 * is hashed directly with the integer finalizer rather than being stringified
 * and hashed as text. The table copies the key; it stores the value pointer as
 * given and never frees the pointed-to object.
 */

#include "ht.h"

#include <stdio.h>
#include <stdlib.h>

/// Copy a uint64 key onto the heap (key-copy callback). The user_data argument
/// is unused.
static void *u64_key_copy(const void *key, void *user_data) {
    (void)user_data;
    uint64_t *k = malloc(sizeof(*k));
    if (!k) {
        perror("ht_u64ptr_insert");
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

/// Wrapper around ht_create that creates a uint64->pointer hash table.
ht_u64ptr_t *ht_u64ptr_create(const ht_u64_options_t *opts) {
    const ht_u64_options_t o = opts ? *opts : (ht_u64_options_t){0};

    /// Keys are copied; values are stored as given (passthrough copy/free).
    const ht_callbacks_t callbacks = {u64_key_copy, u64_key_free, NULL, NULL};

    const ht_options_t base = {
        .hash = o.flooding_resistant ? ht_hash_siphash : ht_hash_int,
        .keyeq = u64_key_eq,
        .keylen = u64_key_len,
        .callbacks = callbacks,
        .key_mode = o.flooding_resistant ? HT_KEY_RANDOM : HT_KEY_NONE,
        .key_best_effort = o.best_effort,
        .initial_capacity = o.initial_capacity,
    };

    return (ht_u64ptr_t *)ht_create(&base);
}

/// Wrapper around ht_destroy for a uint64->pointer hash table.
void ht_u64ptr_destroy(ht_u64ptr_t *ht) { ht_destroy((ht_t *)ht); }

/// Insert a uint64 key mapping to a caller-owned pointer value.
void ht_u64ptr_insert(ht_u64ptr_t *ht, uint64_t key, void *val) {
    ht_insert((ht_t *)ht, &key, val);
}

/// Remove the entry for a uint64 key.
void ht_u64ptr_remove(ht_u64ptr_t *ht, uint64_t key) {
    ht_remove((ht_t *)ht, &key);
}

/// Return the pointer stored under a uint64 key, or NULL if absent.
void *ht_u64ptr_get(ht_u64ptr_t *ht, uint64_t key) {
    return ht_get((ht_t *)ht, &key);
}

/// Wrapper around ht_enum_create for a uint64->pointer hash table.
ht_enum_t *ht_u64ptr_enum_create(ht_u64ptr_t *ht) {
    return ht_enum_create((ht_t *)ht);
}

/// Return the next key/value pair. The value aliases the stored pointer.
bool ht_u64ptr_enum_next(ht_enum_t *he, uint64_t *key, void **val) {
    const void *kp = NULL;
    const void *vp = NULL;
    if (!ht_enum_next(he, &kp, &vp)) {
        return false;
    }

    if (key) {
        *key = *(const uint64_t *)kp;
    }
    if (val) {
        *val = (void *)vp;
    }
    return true;
}

/// Wrapper around ht_enum_destroy for a uint64->pointer enumeration object.
void ht_u64ptr_enum_destroy(ht_enum_t *he) { ht_enum_destroy(he); }

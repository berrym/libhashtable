/**
 * @file ht.c
 * @brief Generic hash table core implementation.
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

#define INITIAL_BUCKETS (16) ///< Initial table size
#define MAX_LOAD_FACTOR                                                        \
    (0.75) ///< Capacity point at which a table needs to grow and rehash
#define MAX_CAPACITY                                                           \
    (1 << 31) ///< Maximum capacity of table when it should not grow and rehash
              ///< (2147483648)
#define GROWTH_FACTOR (2)   ///< Factor by which a table's capacity should grow
#define HT_HASHKEY_MAX (16) ///< Key buffer size for keyed hashes (SipHash key)

typedef struct ht_bucket {
    const void *key;
    const void *val;
    struct ht_bucket *next;
} ht_bucket_t;

/// typedefed to ht_t in ht.h for external scope
struct ht {
    ht_hash hfunc;
    ht_keyeq keyeq;
    ht_keylen keylen;
    ht_callbacks_t callbacks;
    ht_bucket_t *buckets;
    size_t capacity;
    size_t used_buckets;
    unsigned char hashkey[HT_HASHKEY_MAX];
    bool keyed;
    void *user_data;
    size_t grow_count;
};

/// typedefed to ht_enum_t in ht.h for external scope
struct ht_enum {
    ht_t *ht;
    ht_bucket_t *cur;
    size_t idx;
};

/// Default copy callback.
static void *__ht_passthrough_copy(const void *v, void *user_data) {
    (void)user_data;
    return (void *)v;
}

/// Default destroy callback.
static void __ht_passthrough_destroy(const void *v, void *user_data) {
    (void)v;
    (void)user_data;
}

/// Return the table index of a bucket given it's key.
static size_t __ht_bucket_index(const ht_t *ht, const void *key) {
    return ht->hfunc(key, ht->keylen(key), ht->keyed ? ht->hashkey : NULL) %
           ht->capacity;
}

/// Fill a bucket with a key and value.
/// If part of a rehash operation do not make copies of the key value pair.
/// Case 1:
///       Check if the index of the bucket has something already.
///       If not then we add the key and value to the bucket.
///
///  Case 2:
///       Second, if that index already has a key value pair,
///       check if the key is a match and if so replace the value.
///
///  Case 3:
///        We've determined the key isn't in a bucket,
///        but there is something already at that index.
///        Traverse the chain checking each node if we have the key already.
///        If yes, replace the value and we’re done.
///        If not we’ll hit the end and create a new node to add to the
/// chain.
static void __ht_add_to_bucket(ht_t *ht, const void *key, const void *val,
                               bool rehash) {
    ht_bucket_t *cur = NULL, *prev = NULL;
    const size_t idx = __ht_bucket_index(ht, key);

    if (!ht->buckets[idx].key) {
        if (!rehash) {
            key = ht->callbacks.key_copy(key, ht->user_data);

            if (val) {
                val = ht->callbacks.val_copy(val, ht->user_data);
            }
        }

        ht->buckets[idx].key = key;
        ht->buckets[idx].val = val;

        if (!rehash) {
            ht->used_buckets++;
        }
    } else {
        cur = ht->buckets + idx;

        do {
            if (ht->keyeq(key, cur->key)) {
                if (cur->val) {
                    ht->callbacks.val_free(cur->val, ht->user_data);
                }

                if (!rehash && val) {
                    val = ht->callbacks.val_copy(val, ht->user_data);
                }

                cur->val = val;
                prev = NULL;
                break;
            }

            prev = cur;
            cur = cur->next;
        } while (cur);

        if (prev) {
            cur = calloc(1, sizeof(*cur->next));
            if (!cur) {
                perror("__ht_add_to_bucket");
                return;
            }

            if (!rehash) {
                key = ht->callbacks.key_copy(key, ht->user_data);

                if (val) {
                    val = ht->callbacks.val_copy(val, ht->user_data);
                }
            }

            cur->key = key;
            cur->val = val;
            prev->next = cur;

            if (!rehash) {
                ht->used_buckets++;
            }
        }
    }
}

/// Rehash a table growing it's capacity by GROWTH_FACTOR if it has reached
/// MAX_LOAD_FACTOR, but do not grow table if it's capacity has reached
/// MAX_CAPACITY.
static void __ht_rehash(ht_t *ht) {
    ht_bucket_t *buckets = NULL, *cur = NULL, *next = NULL;
    size_t capacity;

    if (ht->used_buckets + 1 < (size_t)(ht->capacity * MAX_LOAD_FACTOR) ||
        ht->capacity >= MAX_CAPACITY) {
        return;
    }

    capacity = ht->capacity;
    buckets = ht->buckets;
    ht->capacity *= GROWTH_FACTOR;
    ht->buckets = calloc(ht->capacity, sizeof(*buckets));
    if (!ht->buckets) {
        perror("__ht_rehash");
        return;
    }
    ht->grow_count++;

    for (size_t i = 0; i < capacity; i++) {
        if (!buckets[i].key) {
            continue;
        }

        __ht_add_to_bucket(ht, buckets[i].key, buckets[i].val, true);

        if (buckets[i].next) {
            cur = buckets[i].next;

            do {
                __ht_add_to_bucket(ht, cur->key, cur->val, true);
                next = cur->next;
                free(cur);
                cur = next;
            } while (cur);
        }
    }

    free(buckets);
    buckets = NULL;
}

/// Create a new hash table of INITIAL_CAPACITY, it requires a hash
/// function, a key equality comparison function, and optionally bucket
/// operations function callbacks structure.
ht_t *ht_create(const ht_options_t *opts) {
    ht_t *ht = NULL;

    if (!opts || !opts->hash || !opts->keyeq || !opts->keylen) {
        return NULL;
    }
    if (opts->key_mode == HT_KEY_PROVIDED && !opts->key) {
        return NULL;
    }

    ht = calloc(1, sizeof(*ht));
    if (!ht) {
        perror("ht_create");
        return NULL;
    }

    ht->hfunc = opts->hash;
    ht->keyeq = opts->keyeq;
    ht->keylen = opts->keylen;
    ht->user_data = opts->user_data;

    ht->callbacks.key_copy = __ht_passthrough_copy;
    ht->callbacks.key_free = __ht_passthrough_destroy;
    ht->callbacks.val_copy = __ht_passthrough_copy;
    ht->callbacks.val_free = __ht_passthrough_destroy;

    if (opts->callbacks.key_copy) {
        ht->callbacks.key_copy = opts->callbacks.key_copy;
    }
    if (opts->callbacks.key_free) {
        ht->callbacks.key_free = opts->callbacks.key_free;
    }
    if (opts->callbacks.val_copy) {
        ht->callbacks.val_copy = opts->callbacks.val_copy;
    }
    if (opts->callbacks.val_free) {
        ht->callbacks.val_free = opts->callbacks.val_free;
    }

    ht->capacity =
        opts->initial_capacity ? opts->initial_capacity : INITIAL_BUCKETS;
    ht->buckets = calloc(ht->capacity, sizeof(*ht->buckets));
    if (!ht->buckets) {
        perror("ht_create");
        free(ht);
        return NULL;
    }

    switch (opts->key_mode) {
    case HT_KEY_NONE:
        break;
    case HT_KEY_PROVIDED:
        memcpy(ht->hashkey, opts->key, HT_HASHKEY_MAX);
        ht->keyed = true;
        break;
    case HT_KEY_RANDOM:
        if (ht_random_bytes(ht->hashkey, HT_HASHKEY_MAX) == 0) {
            ht->keyed = true;
        } else if (opts->key_best_effort) {
            /// Entropy unavailable: proceed with the zeroed key buffer. The
            /// table is usable but a keyed hash is not flooding-resistant.
            ht->keyed = true;
        } else {
            /// Fail closed: a keyed table was requested but the CSPRNG failed.
            free(ht->buckets);
            free(ht);
            return NULL;
        }
        break;
    }

    return ht;
}

/// Destroy a hash table first by freeing all buckets then the table itself.
void ht_destroy(ht_t *ht) {
    ht_bucket_t *next = NULL, *cur = NULL;

    if (!ht) {
        return;
    }

    for (size_t idx = 0; idx < ht->capacity; idx++) {
        if (!ht->buckets[idx].key) {
            continue;
        }

        ht->callbacks.key_free(ht->buckets[idx].key, ht->user_data);
        if (ht->buckets[idx].val) {
            ht->callbacks.val_free(ht->buckets[idx].val, ht->user_data);
        }

        next = ht->buckets[idx].next;
        while (next) {
            cur = next;
            ht->callbacks.key_free(cur->key, ht->user_data);
            if (cur->val) {
                ht->callbacks.val_free(cur->val, ht->user_data);
            }
            next = cur->next;
            free(cur);
            cur = NULL;
        }
    }

    free(ht->buckets);
    ht->buckets = NULL;
    free(ht);
    ht = NULL;
}

/// Insert a key value pair into a table bucket.
void ht_insert(ht_t *ht, const void *key, const void *val) {
    if (!ht || !key) {
        return;
    }

    __ht_rehash(ht);
    __ht_add_to_bucket(ht, key, val, false);
}

/// Remove a bucket from the table.
/// Step 1:
///       Get the bucket index using it's hash.
/// Step 2:
///       Check the bucket and chains for a key match.
/// Step 3:
///       Remove the entry if match is made.
/// Step 4:
///       Relink the chain if necessary.
void ht_remove(ht_t *ht, const void *key) {
    if (!ht || !key) {
        return;
    }

    ht_bucket_t *cur = NULL, *prev = NULL;
    const size_t idx = __ht_bucket_index(ht, key);
    ;

    if (!ht->buckets[idx].key) {
        return;
    }

    if (ht->keyeq(ht->buckets[idx].key, key)) {
        ht->callbacks.key_free(ht->buckets[idx].key, ht->user_data);
        if (ht->buckets[idx].val) {
            ht->callbacks.val_free(ht->buckets[idx].val, ht->user_data);
        }
        ht->buckets[idx].key = NULL;
        ht->buckets[idx].val = NULL;

        cur = ht->buckets[idx].next;
        if (cur) {
            ht->buckets[idx].key =
                ht->callbacks.key_copy(cur->key, ht->user_data);
            if (cur->val) {
                ht->buckets[idx].val =
                    ht->callbacks.val_copy(cur->val, ht->user_data);
            }
            ht->buckets[idx].next = cur->next;
            ht->callbacks.key_free(cur->key, ht->user_data);
            if (cur->val) {
                ht->callbacks.val_free(cur->val, ht->user_data);
            }
            cur->key = NULL;
            cur->val = NULL;
            free(cur);
            cur = NULL;
        }

        ht->used_buckets--;

        return;
    }

    prev = ht->buckets + idx;
    cur = prev->next;

    while (cur) {
        if (ht->keyeq(key, cur->key)) {
            prev->next = cur->next;
            ht->callbacks.key_free(cur->key, ht->user_data);
            if (cur->val) {
                ht->callbacks.val_free(cur->val, ht->user_data);
            }
            cur->key = NULL;
            cur->val = NULL;
            free(cur);
            cur = NULL;
            ht->used_buckets--;
            break;
        }

        prev = cur;
        cur = cur->next;
    }
}

/// Get a table bucket value given it's key and a pointer to store it's
/// value.
static bool __ht_get(const ht_t *ht, const void *key, void **val) {
    if (!ht || !key) {
        return false;
    }

    const ht_bucket_t *cur = NULL;
    const size_t idx = __ht_bucket_index(ht, key);

    if (!ht->buckets[idx].key) {
        return false;
    }

    cur = ht->buckets + idx;
    while (cur) {
        if (ht->keyeq(key, cur->key)) {
            *val = (void *)cur->val;
            return true;
        }
        cur = cur->next;
    }

    return false;
}

/// Get a table bucket value given it's key. It's a wrapper around __ht_get.
void *ht_get(const ht_t *ht, const void *key) {
    void *val = NULL;
    __ht_get(ht, key, &val);
    return val;
}

/// Return the number of entries currently stored in the table.
size_t ht_size(const ht_t *ht) { return ht ? ht->used_buckets : 0; }

/// Return whether a key is present. Unlike comparing ht_get against NULL,
/// this distinguishes an absent key from a key stored with a NULL value.
bool ht_contains(const ht_t *ht, const void *key) {
    void *val = NULL;
    return __ht_get(ht, key, &val);
}

/// Return the value stored under key, first inserting the key/value pair if
/// the key is absent. A single call avoids the time-of-check-to-time-of-use gap
/// of a separate get followed by an insert.
void *ht_get_or_insert(ht_t *ht, const void *key, const void *val) {
    if (!ht || !key) {
        return NULL;
    }

    void *existing = NULL;
    if (__ht_get(ht, key, &existing)) {
        return existing;
    }

    ht_insert(ht, key, val);
    __ht_get(ht, key, &existing);
    return existing;
}

/// Insert a key/value pair, replacing any existing value. Returns true if
/// the key was newly added and false if an existing value was replaced.
bool ht_upsert(ht_t *ht, const void *key, const void *val) {
    if (!ht || !key) {
        return false;
    }

    const bool existed = ht_contains(ht, key);
    ht_insert(ht, key, val);
    return !existed;
}

/// Remove every entry, freeing keys and values through the callbacks, while
/// keeping the table allocated and reusable.
void ht_clear(ht_t *ht) {
    if (!ht) {
        return;
    }

    for (size_t i = 0; i < ht->capacity; i++) {
        if (!ht->buckets[i].key) {
            continue;
        }

        ht->callbacks.key_free(ht->buckets[i].key, ht->user_data);
        if (ht->buckets[i].val) {
            ht->callbacks.val_free(ht->buckets[i].val, ht->user_data);
        }

        ht_bucket_t *cur = ht->buckets[i].next;
        while (cur) {
            ht_bucket_t *next = cur->next;
            ht->callbacks.key_free(cur->key, ht->user_data);
            if (cur->val) {
                ht->callbacks.val_free(cur->val, ht->user_data);
            }
            free(cur);
            cur = next;
        }

        ht->buckets[i].key = NULL;
        ht->buckets[i].val = NULL;
        ht->buckets[i].next = NULL;
    }

    ht->used_buckets = 0;
}

/// Invoke visit(key, val, user_data) for every entry. The table must not be
/// modified during the iteration.
void ht_foreach(const ht_t *ht, ht_visit visit, void *user_data) {
    if (!ht || !visit) {
        return;
    }

    for (size_t i = 0; i < ht->capacity; i++) {
        if (!ht->buckets[i].key) {
            continue;
        }

        visit(ht->buckets[i].key, ht->buckets[i].val, user_data);
        for (const ht_bucket_t *cur = ht->buckets[i].next; cur;
             cur = cur->next) {
            visit(cur->key, cur->val, user_data);
        }
    }
}

/// Fill out with a snapshot of the table's structural statistics. The entry
/// count and grow count are read directly; the per-bucket figures come from a
/// single pass over the buckets, so this never touches the insert, lookup, or
/// remove paths.
void ht_stats(const ht_t *ht, ht_stats_t *out) {
    if (!out) {
        return;
    }

    *out = (ht_stats_t){0};

    if (!ht) {
        return;
    }

    out->size = ht->used_buckets;
    out->capacity = ht->capacity;
    out->grow_count = ht->grow_count;

    for (size_t i = 0; i < ht->capacity; i++) {
        if (!ht->buckets[i].key) {
            continue;
        }

        out->occupied_buckets++;

        size_t chain = 1;
        for (const ht_bucket_t *cur = ht->buckets[i].next; cur;
             cur = cur->next) {
            chain++;
        }
        if (chain > out->max_chain) {
            out->max_chain = chain;
        }
    }

    if (ht->capacity) {
        out->load_factor = (double)ht->used_buckets / (double)ht->capacity;
    }
}

/// Create a table enumeration object.
ht_enum_t *ht_enum_create(ht_t *ht) {
    ht_enum_t *he = NULL;

    if (!ht) {
        return NULL;
    }

    he = calloc(1, sizeof(*he));
    if (!he) {
        perror("ht_enum_create");
        return NULL;
    }
    he->ht = ht;

    return he;
}

/// Get the key value information of the next bucket in a table.
/// Step 1:
///       Iterate through each bucket and if something is there,
///       return the bucket data.
/// Step 2:
///       Move to the next bucket unlese the bucket has a chain,
///       then we store the head of the chain in cur.
///       The next time around we’ll see cur and keep traversing the
///       chain until it's done.
/// Step 4:
///       Once we’ve gone though that chain we’re already pointed to
///       the next bucket and we start over.
bool ht_enum_next(ht_enum_t *he, const void **key, const void **val) {
    const void *mykey = NULL, *myval = NULL;

    if (!he || he->idx >= he->ht->capacity) {
        return false;
    }

    if (!key) {
        key = &mykey;
    }

    if (!val) {
        val = &myval;
    }

    if (!he->cur) {
        while (he->idx < he->ht->capacity && !he->ht->buckets[he->idx].key) {
            he->idx++;
        }

        if (he->idx >= he->ht->capacity) {
            return false;
        }

        he->cur = he->ht->buckets + he->idx;
        he->idx++;
    }

    *key = he->cur->key;
    *val = he->cur->val;
    he->cur = he->cur->next;

    return true;
}

/// Destroy an enumeration object.
void ht_enum_destroy(ht_enum_t *he) {
    if (!he) {
        return;
    }

    free(he);
    he = NULL;
}

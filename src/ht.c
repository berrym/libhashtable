/* ht.c - A generic hash table implementation.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 * License: MIT
 * Copyright (c) Michael Berry <trismegustis@gmail.com> 2024
 */

#include "ht.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define INITIAL_BUCKETS (16) // Initial table size
#define MAX_LOAD_FACTOR                                                        \
    (0.75) // Capacity point at which a table needs to grow and rehash
#define MAX_CAPACITY                                                           \
    (1 << 31) // Maximum capacity of table when it should not grow and rehash
              // (2147483648)
#define GROWTH_FACTOR (2) // Factor by which a table's capacity should grow

typedef struct ht_bucket {
    const void *key;
    const void *val;
    struct ht_bucket *next;
} ht_bucket_t;

struct ht { // typedefed to ht_t in ht.h for external scope
    ht_hash hfunc;
    ht_keyeq keyeq;
    ht_callbacks_t callbacks;
    ht_bucket_t *buckets;
    size_t capacity;
    size_t used_buckets;
#if defined(CPU_32_BIT)
    uint32_t seed;
#else
    uint64_t seed;
#endif
};

struct ht_enum { // typedefed to ht_enum_t in ht.h for external scope
    ht_t *ht;
    ht_bucket_t *cur;
    size_t idx;
};

/**
 * __random_seed:
 *      Generate a random hash offset.
 */
static void __random_seed(ht_t *ht) {
#if defined(CPU_32_BIT)
    uint32_t seed = (uint32_t)time(NULL);
    seed ^= ((uint32_t)ht_create << 16) | (uint32_t)&ht;
    seed ^= (uint32_t)&ht;
#else
    uint64_t seed = (uint64_t)time(NULL);
    seed ^= ((uint64_t)ht_create << 32) | (uint64_t)&ht;
    seed ^= (uint64_t)&ht;
#endif
    ht->seed = seed;
}

/**
 * __default_seed:
 *      Use a default hash offset for FNV1A algorithm.
 */
static void __default_seed(ht_t *ht) { ht->seed = FNV1A_OFFSET; }

/**
 * __ht_passthrough_copy:
 *      Default copy callback.
 */
static void *__ht_passthrough_copy(const void *v) { return (void *)v; }

/**
 * __ht_passthrough_destroy:
 *      Default destroy callback.
 */
static void __ht_passthrough_destroy(const void *v) { return; }

/**
 * __ht_bucket_index:
 *      Return the table index of a bucket given it's key.
 */
static size_t __ht_bucket_index(const ht_t *ht, const void *key) {
    return ht->hfunc(key, ht->seed) % ht->capacity;
}

/**
 * __ht_add_to_bucket:
 *      Fill a bucket with a key and value.
 *      If part of a rehash operation do not make copies of the key value pair.
 *      Case 1:
 *            Check if the index of the bucket has something already.
 *            If not then we add the key and value to the bucket.
 *
 *       Case 2:
 *            Second, if that index already has a key value pair,
 *            check if the key is a match and if so replace the value.
 *
 *       Case 3:
 *             We've determined the key isn't in a bucket,
 *             but there is something already at that index.
 *             Traverse the chain checking each node if we have the key already.
 *             If yes, replace the value and we’re done.
 *             If not we’ll hit the end and create a new node to add to the
 * chain.
 */
static void __ht_add_to_bucket(ht_t *ht, const void *key, const void *val,
                               bool rehash) {
    ht_bucket_t *cur = NULL, *prev = NULL;
    const size_t idx = __ht_bucket_index(ht, key);

    if (!ht->buckets[idx].key) {
        if (!rehash) {
            key = ht->callbacks.key_copy(key);

            if (val) {
                val = ht->callbacks.val_copy(val);
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
                    ht->callbacks.val_free(cur->val);
                }

                if (!rehash && val) {
                    val = ht->callbacks.val_copy(val);
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
                key = ht->callbacks.key_copy(key);

                if (val) {
                    val = ht->callbacks.val_copy(val);
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

/**
 * __ht_rehash:
 *      Rehash a table growing it's capacity by GROWTH_FACTOR if it has reached
 * MAX_LOAD_FACTOR, but do not grow table if it's capacity has reached
 * MAX_CAPACITY.
 */
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

/**
 * ht_create:
 *      Create a new hash table of INITIAL_CAPACITY, it requires a hash
 * function, a key equality comparison function, and optionally bucket
 * operations function callbacks structure.
 */
ht_t *ht_create(const ht_hash hfunc, const ht_keyeq keyeq,
                const ht_callbacks_t *callbacks, const unsigned int flags) {
    ht_t *ht = NULL;

    if (!hfunc || !keyeq) {
        return NULL;
    }

    ht = calloc(1, sizeof(*ht));
    if (!ht) {
        perror("ht_create");
        return NULL;
    }

    ht->hfunc = hfunc;
    ht->keyeq = keyeq;

    ht->callbacks.key_copy = __ht_passthrough_copy;
    ht->callbacks.key_free = __ht_passthrough_destroy;
    ht->callbacks.val_copy = __ht_passthrough_copy;
    ht->callbacks.val_free = __ht_passthrough_destroy;

    if (callbacks) {
        if (callbacks->key_copy) {
            ht->callbacks.key_copy = callbacks->key_copy;
        }
        if (callbacks->key_free) {
            ht->callbacks.key_free = callbacks->key_free;
        }
        if (callbacks->val_copy) {
            ht->callbacks.val_copy = callbacks->val_copy;
        }
        if (callbacks->val_free) {
            ht->callbacks.val_free = callbacks->val_free;
        }
    }

    ht->capacity = INITIAL_BUCKETS;
    ht->buckets = calloc(ht->capacity, sizeof(*ht->buckets));
    if (!ht->buckets) {
        perror("ht_create");
        return NULL;
    }

    if (flags & HT_SEED_RANDOM) {
        __random_seed(ht);
    } else {
        __default_seed(ht);
    }

    return ht;
}

/**
 * ht_destroy:
 *      Destroy a hash table first by freeing all buckets then the table itself.
 */
void ht_destroy(ht_t *ht) {
    ht_bucket_t *next = NULL, *cur = NULL;

    if (!ht) {
        return;
    }

    for (size_t idx = 0; idx < ht->capacity; idx++) {
        if (!ht->buckets[idx].key) {
            continue;
        }

        ht->callbacks.key_free(ht->buckets[idx].key);
        if (ht->buckets[idx].val) {
            ht->callbacks.val_free(ht->buckets[idx].val);
        }

        next = ht->buckets[idx].next;
        while (next) {
            cur = next;
            ht->callbacks.key_free(cur->key);
            if (cur->val) {
                ht->callbacks.val_free(cur->val);
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

/**
 * ht_insert:
 *      Insert a key value pair into a table bucket.
 */
void ht_insert(ht_t *ht, const void *key, const void *val) {
    if (!ht || !key) {
        return;
    }

    __ht_rehash(ht);
    __ht_add_to_bucket(ht, key, val, false);
}

/**
 * ht_remove:
 *      Remove a bucket from the table.
 *      Step 1:
 *            Get the bucket index using it's hash.
 *      Step 2:
 *            Check the bucket and chains for a key match.
 *      Step 3:
 *            Remove the entry if match is made.
 *      Step 4:
 *            Relink the chain if necessary.
 */
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
        ht->callbacks.key_free(ht->buckets[idx].key);
        if (ht->buckets[idx].val) {
            ht->callbacks.val_free(ht->buckets[idx].val);
        }
        ht->buckets[idx].key = NULL;
        ht->buckets[idx].val = NULL;

        cur = ht->buckets[idx].next;
        if (cur) {
            ht->buckets[idx].key = ht->callbacks.key_copy(cur->key);
            if (cur->val) {
                ht->buckets[idx].val = ht->callbacks.val_copy(cur->val);
            }
            ht->buckets[idx].next = cur->next;
            ht->callbacks.key_free(cur->key);
            if (cur->val) {
                ht->callbacks.val_free(cur->val);
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
            ht->callbacks.key_free(cur->key);
            if (cur->val) {
                ht->callbacks.val_free(cur->val);
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

/**
 * __ht_get:
 *      Get a table bucket value given it's key and a pointer to store it's
 * value.
 */
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

/**
 * ht_get:
 *      Get a table bucket value given it's key. It's a wrapper around __ht_get.
 */
void *ht_get(const ht_t *ht, const void *key) {
    void *val = NULL;
    __ht_get(ht, key, &val);
    return val;
}

/**
 * ht_enum_create:
 *      Create a table enumeration object.
 */
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

/**
 * ht_enum_next:
 *      Get the key value information of the next bucket in a table.
 *      Step 1:
 *            Iterate through each bucket and if something is there,
 *            return the bucket data.
 *      Step 2:
 *            Move to the next bucket unlese the bucket has a chain,
 *            then we store the head of the chain in cur.
 *            The next time around we’ll see cur and keep traversing the
 *            chain until it's done.
 *      Step 4:
 *            Once we’ve gone though that chain we’re already pointed to
 *            the next bucket and we start over.
 */
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

/**
 * ht_enum_destroy:
 *      Destroy an enumeration object.
 */
void ht_enum_destroy(ht_enum_t *he) {
    if (!he) {
        return;
    }

    free(he);
    he = NULL;
}

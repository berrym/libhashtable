/**
 * @file entity_index.c
 * @brief map 64-bit handles to records (uint64 -> pointer).
 *
 * The flagship demonstration of type-matched hashing. A uint64_t handle keys a
 * record directly, with no stringification: ht_u64ptr hashes the integer with
 * the integer finalizer, which is the correct hash for integer/identity keys
 * (FNV-1a's per-byte loop is a poor fit). The table stores borrowed pointers;
 * the caller owns the records.
 *
 * Project: libhashtable
 * @license MIT
 */

#include "ht.h"

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint64_t id;
    const char *name;
    int hp;
} entity_t;

int main(void) {
    entity_t entities[] = {
        {0x1000000000000001ULL,   "player", 100},
        {0x1000000000000002ULL,   "goblin",  12},
        {0x2000000000000003ULL, "merchant",  30},
    };
    const size_t count = sizeof(entities) / sizeof(entities[0]);

    ht_u64ptr_t *index = ht_u64ptr_create(NULL);
    if (!index) {
        return 1;
    }

    for (size_t i = 0; i < count; i++) {
        ht_u64ptr_insert(index, entities[i].id, &entities[i]);
    }

    const uint64_t handle = 0x1000000000000002ULL;
    const entity_t *e = ht_u64ptr_get(index, handle);
    if (e) {
        printf("handle %016llx -> %s (hp %d)\n", (unsigned long long)handle,
               e->name, e->hp);
    }

    ht_u64ptr_destroy(index);
    return 0;
}

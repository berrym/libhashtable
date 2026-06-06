/**
 * @file record_store.c
 * @brief store packed binary records (string -> blob).
 *
 * Demonstrates ht_strblob: a string key maps to an arbitrary byte buffer that
 * may contain embedded NUL bytes (here a packed fixed-size struct with a zero
 * flags byte), which a string-valued table would truncate at the first NUL.
 *
 * Project: libhashtable
 * @license MIT
 */

#include "ht.h"

#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint32_t version;
    uint8_t flags; ///< may be 0x00
    uint16_t length;
} record_t;

int main(void) {
    ht_strblob_t *store = ht_strblob_create(NULL);
    if (!store) {
        return 1;
    }

    const record_t rec = {.version = 3, .flags = 0x00, .length = 512};
    ht_strblob_insert(store, "config/main", &rec, sizeof(rec));

    size_t size = 0;
    const record_t *got = ht_strblob_get(store, "config/main", &size);
    if (got && size == sizeof(*got)) {
        printf("config/main: version=%u flags=0x%02x length=%u (%zu bytes)\n",
               got->version, got->flags, got->length, size);
    }

    ht_strblob_destroy(store);
    return 0;
}

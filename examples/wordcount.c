/* wordcount - count word frequencies from stdin (string -> int).
 *
 * Demonstrates ht_strint: short, trusted text keys map to integer counts. The
 * default hash is FNV-1a, which suits short string keys. ht_strint_get returns
 * a mutable pointer into the table's storage, so an existing count is bumped in
 * place.
 *
 * Project: libhashtable
 * License: MIT
 */

#include "ht.h"

#include <ctype.h>
#include <stdio.h>

int main(void) {
    ht_strint_t *counts = ht_strint_create(NULL);
    if (!counts) {
        return 1;
    }

    char word[256];
    size_t len = 0;
    int c;
    do {
        c = getchar();
        if (c != EOF && isalnum((unsigned char)c)) {
            if (len + 1 < sizeof(word)) {
                word[len++] = (char)tolower((unsigned char)c);
            }
        } else if (len > 0) {
            word[len] = '\0';
            int *cur = ht_strint_get(counts, word);
            if (cur) {
                (*cur)++;
            } else {
                int one = 1;
                ht_strint_insert(counts, word, &one);
            }
            len = 0;
        }
    } while (c != EOF);

    ht_enum_t *e = ht_strint_enum_create(counts);
    const char *key = NULL;
    const int *val = NULL;
    while (ht_strint_enum_next(e, &key, &val)) {
        printf("%7d  %s\n", *val, key);
    }
    ht_strint_enum_destroy(e);

    ht_strint_destroy(counts);
    return 0;
}

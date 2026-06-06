/* request_router - route untrusted path keys with flooding resistance.
 *
 * Demonstrates the security tier of the menu. The keys are untrusted request
 * paths, so the table is created flooding_resistant, which selects keyed
 * SipHash with a per-table random key. An attacker cannot engineer key
 * collisions to degrade lookups to O(n). Creation fails closed if the system
 * has no entropy source.
 *
 * Project: libhashtable
 * License: MIT
 */

#include "ht.h"

#include <stdio.h>

int main(void) {
    ht_strstr_t *routes =
        ht_strstr_create(&(ht_str_options_t){.flooding_resistant = true});
    if (!routes) {
        fprintf(stderr, "router creation failed (no entropy source?)\n");
        return 1;
    }

    ht_strstr_insert(routes, "/", "home");
    ht_strstr_insert(routes, "/login", "auth");
    ht_strstr_insert(routes, "/api/users", "users");

    const char *paths[] = {"/login", "/missing", "/api/users"};
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        const char *handler = ht_strstr_get(routes, paths[i]);
        printf("%-12s -> %s\n", paths[i], handler ? handler : "(404)");
    }

    ht_strstr_destroy(routes);
    return 0;
}

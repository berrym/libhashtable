# libhashtable

## Description

A small, general-purpose hash table library in C (C11 + POSIX). It pairs a
generic `void *` core with type-safe wrappers and a curated menu of
type-matched hash algorithms, so the right hash is used for the shape of the
keys — short strings, integers, long blobs, or untrusted input.

## Getting Started

Install the meson and ninja build systems, then clone and build:

```
git clone https://github.com/berrym/libhashtable.git
meson setup buildDir
meson compile -C buildDir
```

Run the test suite:

```
meson test -C buildDir
```

Build the example programs (off by default):

```
meson setup buildDir -Dexamples=true
meson compile -C buildDir
```

## Usage

The typed wrappers are the easy path — each pairs a key/value type with a
sensible default hash:

```c
#include "ht.h"

ht_strint_t *counts = ht_strint_create(NULL); /* case-sensitive, FNV-1a */
int n = 1;
ht_strint_insert(counts, "apple", &n);
const int *got = ht_strint_get(counts, "apple");
ht_strint_destroy(counts);
```

The generic core is constructed from an options struct; zero-initialized fields
take sensible defaults (passthrough callbacks, unkeyed, default capacity):

```c
ht_t *t = ht_create(&(ht_options_t){
    .hash = ht_hash_fnv1a,
    .keyeq = str_eq,
    .keylen = str_len,
    .callbacks = {.key_copy = my_key_copy, .key_free = my_key_free},
});
```

For untrusted keys, request a flooding-resistant table, which selects keyed
SipHash with a per-table random key:

```c
ht_strstr_t *routes =
    ht_strstr_create(&(ht_str_options_t){.flooding_resistant = true});
```

## Hash algorithm menu

| Function                              | Best for                                  |
|---------------------------------------|-------------------------------------------|
| `ht_hash_fnv1a` / `ht_hash_fnv1a_casecmp` | short strings                         |
| `ht_hash_int`                         | integers, pointers, identity keys         |
| `ht_hash_bulk`                        | long strings and binary blobs (wyhash)    |
| `ht_hash_siphash` / `ht_hash_siphash24` | untrusted keys (keyed SipHash 1-3 / 2-4) |

Hash values are 64-bit (`ht_hash_t` is `uint64_t`). The hash signature is
`uint64_t (*)(const void *key, size_t len, const void *hashkey)`; `hashkey` is
the keying material for keyed PRFs and NULL for unkeyed hashes.

## Typed wrappers

- `ht_strstr`, `ht_strint`, `ht_strfloat`, `ht_strdouble` — string key to a
  typed value.
- `ht_strblob` — string key to a binary value (embedded NUL bytes preserved).
- `ht_strptr` — string key to a caller-owned pointer.
- `ht_strset` — a set of strings (membership only).
- `ht_u64ptr`, `ht_u64blob` — 64-bit integer key to a pointer or binary value,
  hashed with the integer finalizer.

String-keyed wrappers take an `ht_str_options_t` (`case_insensitive`,
`flooding_resistant`, `best_effort`, `initial_capacity`); integer-keyed
wrappers take an `ht_u64_options_t`. A NULL options pointer selects the
defaults.

## Compound operations

In addition to `ht_insert` / `ht_get` / `ht_remove`, the core provides
`ht_size`, `ht_contains`, `ht_get_or_insert`, `ht_upsert`, `ht_clear`, and
`ht_foreach`.

## Thread safety

The library holds no hidden shared mutable state and reads never mutate, so it
is safe under caller (external) locking. The compound operations are
single-call, which lets a caller holding its own lock avoid
time-of-check-to-time-of-use sequences. There is no built-in lock.

## Version

v0.8.0

## Authors

Copyright 2024 Michael Berry <trismegustis@gmail.com>

## License

This project is licensed under the MIT License - see the LICENSE file for details.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![build result](https://build.opensuse.org/projects/home:berrym/packages/libhashtable-devel/badge.svg?type=default)](https://build.opensuse.org/package/show/home:berrym/libhashtable-devel)
[![Copr build status](https://copr.fedorainfracloud.org/coprs/mberry/libhashtable-devel/package/libhashtable-devel/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/mberry/libhashtable-devel/package/libhashtable-devel/)

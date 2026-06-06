# libhashtable 0.8.0 — Clean Core Design (Blueprint)

Status: BLUEPRINT for the 0.8.0 rewrite (2026-06-05). This supersedes the
incremental `libhashtable-pluggable-hash-design.md`, which remains as the record
of how the decisions were reached. The original (pre-0.8.0) construction and
configuration design is treated as **deprecated**.

Guiding stance: rewrite the **architecture** cleanly; carry forward the
**verified primitives** unchanged; leave no leftover flag/seed cruft from the
FNV-only era. The functional core already built and tested is an asset; the
construction/configuration/threading/wrapper *skin* is what gets redesigned.

## 1. Scope and identity

libhashtable is a **general-purpose hash-table library**, not a hashing toolkit.
The hash functions exist to serve the table; the hash value is therefore
unconditionally **64-bit** and SipHash's 128-bit output is out of scope.
(CONFIRM: hash-table library, not hashing toolkit.)

Platform posture: **platform-independent ISO C core**; all platform-specific
code is isolated to single translation units. Windows is supported because its
only cost is one isolated entropy backend (`BCryptGenRandom`) — it is not a
source of design complexity. (CONFIRM: keep Windows support.)

## 2. Language and portability baseline

- **C11 + POSIX.** `c_std=c11`. The current code is C99-expressible at the
  language level, but C11 is the forward baseline (compound ops and a future
  thread-safety story benefit; `_Static_assert` and anonymous unions are used
  deliberately). The real non-ISO-C dependency is POSIX, isolated to entropy and
  `strdup`/`strcasecmp`.
- **Feature-test macros:** `-D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE`, set once in
  `meson.build`. Both are required: on glibc >= 2.19, defining `_XOPEN_SOURCE`
  alone suppresses the default set, so `_DEFAULT_SOURCE` must accompany it. No
  `_GNU_SOURCE`, no `_POSIX_C_SOURCE` — not needed. (Fixes the `XOPEN=600`
  no-op typo.)
- **C11 features used:** `_Static_assert` (e.g. assert the key buffer is large
  enough for the keyed PRF), anonymous unions/structs (group option variants).
- **No GNU/GCC extensions** in first-party code; vendored hashes use their
  portable paths only (no `__uint128_t`, no `__builtin_*`).

## 3. Core hash interface (carried forward)

```c
typedef uint64_t ht_hash_t;                                  /// fixed 64-bit
typedef ht_hash_t (*ht_hash)(const void *key, size_t len, const void *hashkey);
typedef bool   (*ht_keyeq)(const void *a, const void *b);
typedef size_t (*ht_keylen)(const void *key);
```

`key + len` supports binary keys; `hashkey` carries keyed-PRF key material (NULL
for unkeyed hashes). The base engine obtains each key's length via the `keylen`
callback (Option B), so `ht_insert`/`ht_get`/`ht_remove` and the key/value
callbacks keep simple signatures and no per-entry length is stored.

## 4. Construction API — options struct (the central rewrite)

The flag bitfield (`ht_flags_enum_t`) and the positional `ht_create(...)` are
**removed**. Construction takes one options struct, extensible by appending
fields without breaking call sites:

```c
typedef enum {
    HT_KEY_NONE = 0,  /// unkeyed hash; hashkey is NULL
    HT_KEY_RANDOM,    /// generate a per-table key from the CSPRNG
    HT_KEY_PROVIDED,  /// caller supplies the key material
} ht_key_mode;

typedef struct {
    ht_hash        hash;             /// required
    ht_keyeq       keyeq;            /// required
    ht_keylen      keylen;           /// required
    ht_callbacks_t callbacks;        /// zero-initialized => passthrough
    ht_key_mode    key_mode;         /// default HT_KEY_NONE
    const void    *key;              /// HT_KEY_PROVIDED: 16 bytes
    bool           key_best_effort;  /// HT_KEY_RANDOM: degrade vs fail on CSPRNG failure
    size_t         initial_capacity; /// 0 => default
} ht_options_t;

ht_t *ht_create(const ht_options_t *opts);
```

Call sites use C99 designated initializers:

```c
ht_t *t = ht_create(&(ht_options_t){
    .hash = ht_hash_fnv1a, .keyeq = str_eq, .keylen = str_len,
    .callbacks = { .key_copy = ..., ... },
});
```

What this dissolves (the "goofy decisions"):
- **Case sensitivity** is no longer a flag — it is which hash/keyeq you pass
  (`ht_hash_fnv1a` vs `ht_hash_fnv1a_casecmp`).
- **Keying** is explicit via `key_mode`, not an overloaded `HT_SEED_RANDOM` bit.
- **Entropy-failure policy** is `key_best_effort`, not a separate flag bit.
- The `HT_STR_*`/`HT_SEED_*` enum and all bit-composition concerns are gone.

Validation: `ht_create` returns NULL if `hash`/`keyeq`/`keylen` are NULL, if
`key_mode == HT_KEY_PROVIDED` with a NULL `key`, or if `key_mode == HT_KEY_RANDOM`
and the CSPRNG fails while `key_best_effort` is false (fail-closed). A keyed hash
(SipHash) requires `key_mode != HT_KEY_NONE`; this is documented at the base
layer and enforced by the wrappers.

## 5. Algorithm menu (carried forward, verified)

| Function | Key shape | Keyed | Status |
|---|---|---|---|
| `ht_hash_fnv1a` / `_casecmp` | short strings | no | verified |
| `ht_hash_int` | integers / pointers / identity | no | verified |
| `ht_hash_bulk` (wyhash) | long strings / binary blobs | no | vendored, vectors |
| `ht_hash_siphash` (1-3) / `ht_hash_siphash24` (2-4) | adversarial keys | yes | vendored, vectors |

All retained as-is with their reference-vector tests. Vendored sources keep
provenance headers and licenses.

## 6. Keying and entropy (carried forward)

`int ht_random_bytes(void *buf, size_t len)` — one-TU platform dispatch
(getrandom / `/dev/urandom`, arc4random_buf, BCryptGenRandom), hard-fail, never
weak substitution. `HT_KEY_RANDOM` fills the table's 16-byte key buffer
(`_Static_assert`ed >= the SipHash key size) at create; the per-table read needs
no caching, so there is no fork hazard. No unkeyed randomization exists — it is
not a flooding defense (research-confirmed).

## 7. Thread-safety model

Thread-safety is a **design discipline**, enforced, not a bolt-on:
- Zero hidden shared mutable state; reads never mutate; fully reentrant. The
  vendored hashes are pure over read-only `static const` data; `ht_random_bytes`
  reads per-call with no shared fd. This makes **caller (external) locking
  correct and sufficient** — the model lush already uses.
- **Compound atomic operations** are core API so external-lockers avoid
  TOCTOU two-call sequences: `ht_get_or_insert`, `ht_upsert`, `ht_contains`,
  `ht_size`, `ht_clear`, `ht_foreach`.
- **No built-in lock in 0.8.0.** A per-table lock only serves a narrow slice
  (whole-table serialization for a user who would rather not declare a mutex),
  at the cost of a permanent portable threading backend and per-op locking, with
  no evidence of demand (lush external-locks). It is **purely additive later**
  via an options `lock_mode` field if real demand appears; deferring keeps 0.8.0
  free of any threading dependency.

Documented concurrency contract: safe under caller locking; use the compound
operations to avoid two-call races.

## 8. Typed-wrapper menu

Wrappers fill an `ht_options_t` internally and re-impose compile-time type
safety. Each takes a small, named wrapper-options struct (NULL => defaults) —
no flag puns:

```c
typedef struct {
    bool   case_insensitive;   /// use the casecmp hash + comparison
    bool   flooding_resistant; /// keyed SipHash with a random key
    bool   best_effort;        /// flooding_resistant: degrade vs fail
    size_t initial_capacity;
} ht_str_options_t;
```

`case_insensitive` and `flooding_resistant` are mutually exclusive (SipHash has
no case-folding variant); the combination returns NULL — an explicit, validated
error rather than a silent flag collision. (A case-folding keyed hash could be
added later if needed.)

Curated wrappers (existing string-keyed set carried forward; new ones by
evidence strength):

| Wrapper | Key | Value | Default hash |
|---|---|---|---|
| `ht_strstr` / `ht_strint` / `ht_strfloat` / `ht_strdouble` | string | typed | FNV-1a |
| `ht_strblob` | string | binary blob | FNV-1a |
| `ht_strptr` | string | owned pointer | FNV-1a |
| `ht_strset` | string | presence | FNV-1a |
| `ht_u64ptr` / `ht_u64blob` | uint64 | pointer / blob | `ht_hash_int` |

`flooding_resistant` selects SipHash for any string wrapper, with the table
supplying the random key — the honest replacement for the old `HT_SEED_RANDOM`.

## 9. What carries forward vs. what is removed

Carried forward (assets): the `(key,len,hashkey)` interface, the `keylen`
callback, all four vetted algorithms with their reference-vector tests,
`ht_random_bytes`, the chain/bucket engine, the void*-core + typed-wrapper
architecture.

Removed (deprecated baggage): `ht_flags_enum_t` and all bit-composition; the
`HT_SEED_RANDOM`/`HT_SEED_BEST_EFFORT`/`HT_STR_CASECMP` flag mechanics;
positional `ht_create`; the flag-driven wrapper idiom; the `XOPEN=600` typo.
Also fix the pre-existing `ht_create` leak (the `!ht->buckets` path leaks `ht`).

## 10. Examples (demos)

A generic `examples/` set (built behind a meson option): `wordcount` (string->int),
`entity_index` (uint64->pointer, flagship), `record_store` (string->blob),
`request_router` (SipHash over untrusted keys). Generic domains, never
shell-flavored.

## 11. Sequencing of the rewrite

1. Baseline: meson feature-test fix; `_Static_assert` scaffolding.
2. Construction: `ht_options_t` + `ht_create(const ht_options_t *)`; remove the
   flag enum; retrofit the engine and the four existing wrappers; fix the leak.
3. Compound operations: `ht_get_or_insert`, `ht_upsert`, `ht_contains`,
   `ht_size`, `ht_clear`, `ht_foreach`.
4. Typed-wrapper menu: `ht_strblob` -> `ht_u64ptr`/`ht_u64blob` -> `ht_strptr`
   -> `ht_strset`, each on the options-struct foundation.
5. Examples.

Each step builds clean, formats, and tests with no warnings, per workflow
hygiene. Verified primitives are not re-derived.

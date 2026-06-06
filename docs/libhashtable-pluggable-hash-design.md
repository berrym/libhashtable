# libhashtable: Pluggable, Type-Matched Hashing — Design Proposal

Status: DRAFT for review (2026-06-03). Nothing implemented. Open decisions are
collected in §10 and must be signed off before any code lands.

This proposal is framed for a general-purpose library serving ANY consumer. The
lush shell is cited only as one evidence source; where the evidence is silent,
that is recorded explicitly and is NOT a reason to narrow the design.

## 1. Goals and non-goals

Goals:

- Make the hash algorithm hot-swappable at the core layer, with a curated menu
  of type-matched algorithms delivered through typed wrappers.
- Support binary keys (embedded NULs, fixed-size records, integers-by-value),
  which the current NUL-terminated interface cannot hash.
- Support genuine attacker resistance (keyed PRF) for consumers that face
  untrusted key input, generated from a real CSPRNG.
- Keep the public API platform-neutral; isolate all platform code to one
  translation unit.
- Preserve the void* core plus typed-wrapper architecture and its compile-time
  type checking.

Non-goals:

- Imposing a security policy. The library exposes an interface rich enough that
  each consumer chooses speed, type-fit, or attacker resistance. It does not
  force SipHash on consumers that do not need it, nor withhold it from those
  that do.
- Serving any single downstream. The menu includes entries this corpus does not
  exercise, because a general-purpose library must serve consumers absent from
  any one evidence set.

## 2. The core interface change

Current:

```c
typedef ht_hash_t (*ht_hash)(const void *, ht_hash_t);   /// (key, seed)
```

Two structural limits, both proven by the research and the consumer audit:

- No length parameter — `__fnv1a_hash` loops `for (p = key; *p; p++)`, so the
  "generic" void* core is really NUL-terminated-bytes-generic. It cannot hash
  binary keys, and it forces integer keys to be stringified.
- The seed is a single scalar — a keyed PRF (SipHash) needs ~128 bits of key,
  which a single `ht_hash_t` cannot carry. This is why the old `__random_seed`
  resorted to XOR-ing a weak offset into FNV's accumulator.

Proposed:

```c
/// A hash function over `len` bytes at `key`, optionally keyed by `hashkey`.
/// `hashkey` points to keying material for keyed PRFs (e.g. SipHash's 16-byte
/// key) and is NULL for unkeyed hashes, which must ignore it. Returns a 64-bit
/// hash; the table reduces it to a bucket index.
typedef uint64_t (*ht_hash)(const void *key, size_t len, const void *hashkey);
```

`key + len` handles binary keys. `hashkey` carries PRF key material. The closest
real-world precedent is Python's `PyHash_FuncDef` (PEP 456), which pairs a
length-carrying signature with a metadata struct; the key-only callbacks in
Redis `dict`, GLib `GHashFunc`, CCAN htable, and khashl all lack length and are
the cautionary cases.

## 3. Hash output width — retire HT_HASH_WIDTH

Recommendation: make the hash value unconditionally 64-bit and retire
`HT_HASH_WIDTH` as a build knob.

Rationale:

- Every modern menu algorithm (wyhash, xxh3, SipHash) is 64-bit native. A
  32-bit table still reduces a 64-bit hash to a bucket index by `% capacity`.
- `HT_HASH_WIDTH` exists only to select the `ht_hash_t` typedef and the FNV
  constant pair. With a fixed 64-bit output, `ht_hash_t` becomes `uint64_t`
  unconditionally and FNV-1a always uses its 64-bit constants. A 32-bit FNV can
  still ship as an explicit menu entry for consumers who want it, but it is no
  longer the axis the whole library pivots on.
- This continues the simplification already begun by removing the legacy
  `CPU_32_BIT`/`CPU_64_BIT` arms: width selection collapses from
  "CPU macros / HT_HASH_WIDTH / UINTPTR_MAX autodetect" down to "there is one
  width, 64-bit."

Consequence: `ht_hash_t` is retained as a name (`typedef uint64_t ht_hash_t;`)
for readability, but it is no longer configurable.

## 4. Key material and seeding

The single scalar seed is replaced by an optional table-owned key buffer.

```c
#define HT_HASHKEY_MAX 16   /// bytes; sized for SipHash's 128-bit key

struct ht {
    ht_hash hfunc;
    /// ... existing fields ...
    unsigned char hashkey[HT_HASHKEY_MAX];   /// keying material; zeroed if unkeyed
    bool keyed;                              /// true once filled from the CSPRNG
};
```

Behavior:

- A keyed hash (SipHash) requested with `HT_SEED_RANDOM` causes `ht_create` to
  call `ht_random_bytes(ht->hashkey, HT_HASHKEY_MAX)` once at creation. Each
  hash call receives `ht->hashkey`.
- An unkeyed hash receives NULL (or the zeroed buffer) and ignores it.
- The old single-`ht_hash_t` "random offset" for FNV is REMOVED. The research is
  unambiguous: randomizing the seed/offset of an unkeyed hash does not resist
  flooding (Murmur was broken even when seeded). Reintroducing it would be
  security theater. Randomization is meaningful ONLY through a keyed PRF.

This deletes `__random_seed` entirely — including its non-portable
function-pointer-to-integer casts and the pointer-truncation warnings that first
surfaced this work. The feature it pretended to provide is now provided for real
by SipHash keying, or honestly not at all.

Failure policy (from the entropy research): `ht_random_bytes` hard-fails rather
than emit weak bytes, but `ht_create` must NOT abort the host process the way a
crypto library would. Instead the table layer decides: fail the create, or fall
back to the unkeyed default hash and signal it to the caller. A consumer that
asked for attacker resistance learns it did not get it, instead of receiving
silently-weak keying. Proposed: `ht_create` returns NULL on entropy failure when
a keyed hash was explicitly requested; a separate flag selects graceful
degradation for consumers that prefer a working unkeyed table over no table.

## 5. The algorithm menu

Exported base-layer hash functions any consumer may pass to `ht_create`:

| Function (proposed)     | Key shape it serves            | Evidence status        |
|-------------------------|--------------------------------|------------------------|
| `ht_hash_fnv1a`         | short strings                  | VALIDATED              |
| `ht_hash_fnv1a_casecmp` | short strings, case-insensitive| VALIDATED              |
| `ht_hash_int`           | integers / pointers / identity | STRONGLY VALIDATED     |
| `ht_hash_bulk`          | long strings / binary blobs    | first-principles only  |
| `ht_hash_siphash`       | adversarial keys (keyed PRF)   | first-principles only  |

Evidence status is from the lush-shape audit:

- VALIDATED — the dominant real shape; FNV-1a is correct for short trusted
  identifier strings.
- STRONGLY VALIDATED — two independent `uint64_t`-keyed sites; one consumer
  hand-rolled Thomas Wang's integer mix (an integer finalizer) specifically to
  escape FNV-1a, because no integer-keyed path existed. A real consumer
  independently arrived at exactly the prescribed algorithm.
- first-principles only — the corpus has no long/binary KEYS (blobs appear only
  as VALUES) and no attacker-controlled key surface. These entries serve
  consumers absent from this corpus (network/web handlers parsing untrusted
  input; large-key consumers). They remain on the menu by design.

Algorithm choices:

- `ht_hash_int`: a single integer finalizer (splitmix64 / fmix64). For a key
  already known to be an integer or pointer, this avoids FNV's per-byte loop and
  its poor avalanche on small values.
- `ht_hash_bulk`: recommend wyhash — tiny, public domain, portable C, no SIMD
  dependency. xxh3 is faster on large inputs but larger and SIMD-leaning; see
  §10.
- `ht_hash_siphash`: recommend SipHash-1-3 as the keyed default — it is the
  modern hash-flooding choice (Rust), faster than 2-4, with adequate margin for
  table keying. SipHash-2-4 (Python's choice) is the conservative option; see
  §10. Vendored from the CC0 reference implementation.

## 6. The typed-wrapper menu

Typed wrappers pair a key/value type with its best-fit default algorithm and
re-impose compile-time type checking. The base layer stays override-capable.

Existing (string-keyed, retained): `ht_strstr`, `ht_strint`, `ht_strfloat`,
`ht_strdouble` — all use FNV-1a; VALIDATED.

Proposed additions:

| Wrapper        | Key      | Value        | Default hash    | Evidence / motivation                         |
|----------------|----------|--------------|-----------------|-----------------------------------------------|
| `ht_strptr`    | string   | void* (owned)| FNV-1a          | VALIDATED — kills `%p` hex-string smuggling (2 sites) |
| `ht_strblob`   | string   | binary blob  | FNV-1a          | VALIDATED — land upstream; binary-safe values; fix the `HT_SEED_RANDOM`-ignored bug |
| `ht_strset`    | string   | (presence)   | FNV-1a          | VALIDATED — presence-only set variant (1 site) |
| `ht_u64ptr`    | uint64_t | void* (owned)| `ht_hash_int`   | STRONGLY VALIDATED — the history-index shape  |
| `ht_u64blob`   | uint64_t | binary blob  | `ht_hash_int`   | VALIDATED — the render-cache shape (integer key + binary value) |

Naming follows the existing `ht_<keytype><valtype>` convention (`ht_strstr` =
string key, string value; so `ht_u64ptr` = uint64 key, pointer value).

Keyed/adversarial use is orthogonal to the wrapper: any wrapper created with
`HT_SEED_RANDOM` and the SipHash hash becomes flooding-resistant. A consumer
parsing untrusted string keys uses `ht_strstr` + SipHash; one with trusted keys
uses the FNV-1a default.

## 7. Entropy module

A single translation unit (`src/ht_random.c`) holds all platform `#if defined`
selection behind one neutral function:

```c
/// Fill `buf` with `len` cryptographically-strong bytes. Returns 0 on success,
/// -1 on failure. On failure NO bytes of `buf` are considered valid; the
/// function never silently degrades to weak entropy.
int ht_random_bytes(void *buf, size_t len);
```

Backend selection order (compile-time):

- Linux: `getrandom(buf, len, 0)`; fall back to `/dev/urandom` only on `ENOSYS`
  (pre-3.17 kernels).
- macOS / BSD: `arc4random_buf()` (never-fails contract) or `getentropy()`.
- Windows: `BCryptGenRandom(NULL, buf, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG)`.
- `/dev/urandom` fallback: open `O_RDONLY | O_CLOEXEC`, verify `S_ISCHR`, loop
  partial reads (the no-short-read guarantee is `getrandom(2)`'s, not the file's
  — short reads occur on gVisor), retry `EINTR`/`EAGAIN`. Never `/dev/random`.

ISO-C last resort: C99/C11 provide no CSPRNG and `rand()`/`random()` are unfit
for keying. Both libsodium and Rust's getrandom hard-fail rather than emit weak
bytes. Default here is hard-fail. A weak `time`/`pid`/address mix exists only as
an explicit opt-in for consumers that prefer defense-in-depth over a hard error.

Fork safety: a non-issue for this design. SipHash needs 16 bytes once per table
at creation — well under every platform's per-call limit, no looping. The table
does a per-table read and caches nothing, so the `fork()` duplicate-key hazard
never arises.

## 8. Backward compatibility and migration

The `ht_hash` signature and the seeding model change incompatibly. This is a
breaking change at the core layer.

Insulation: the typed wrappers (`ht_strstr` and friends) do not expose the hash
function, so consumers that only use wrappers are source-compatible except for
the removal of the unkeyed random-offset behavior. Consumers that call the base
`ht_create` with their own hash must update.

Migration for base-layer consumers:

- Old hash `ht_hash_t f(const void *key, ht_hash_t seed)` becomes
  `uint64_t f(const void *key, size_t len, const void *hashkey)`.
- A hand-rolled integer hash (e.g. a Thomas Wang mix over a `uint64_t` key) is
  replaced by the `ht_u64*` wrapper or by passing `ht_hash_int`.
- `fnv1a_hash_str` / `fnv1a_hash_str_casecmp` gain a `len` parameter and lose the
  seed; string wrappers compute `strlen` internally.

Recommendation: a clean break at version 0.8.0 (pre-1.0 signals a breaking
change), with a migration table in the release notes, and no compatibility shim.
The consumer base is small and vendors the source, so it can update in lockstep;
a shim would carry the exact dead-weight the `CPU_*_BIT` cleanup just removed.
See §10 for sign-off.

## 9. File and layering organization

```
include/ht.h            public API (platform-neutral)
src/ht.c                core table
src/ht_random.c         entropy dispatch (all platform #ifdefs isolated here)
src/hash/ht_fnv1a.c     FNV-1a (+ casecmp)
src/hash/ht_int.c       integer finalizer
src/hash/ht_bulk.c      wyhash (vendored)
src/hash/ht_siphash.c   SipHash (vendored, CC0 reference)
src/wrap/ht_str*.c      string-keyed typed wrappers
src/wrap/ht_u64*.c      integer-keyed typed wrappers
```

All vendored algorithms are portable C, no GNU/GCC extensions, permissive or
public-domain licensed (wyhash: public domain; SipHash reference: CC0). This
keeps the dependency-free, vendor-clean posture intact.

## 10. Open decisions requiring sign-off

These are independent and will be taken one at a time, highest-priority first.

1. Hash output width — retire `HT_HASH_WIDTH` and make the hash value
   unconditionally 64-bit (`ht_hash_t = uint64_t`)? (Recommended: yes.)
   RESOLVED 2026-06-03: YES. `ht_hash_t` becomes a fixed `typedef uint64_t`;
   `HT_HASH_WIDTH` and the meson `hash_width` option (`meson_options.txt` plus
   its `add_project_arguments` block in `meson.build`) are removed; FNV-1a uses
   its 64-bit constants, with a 32-bit FNV available only as an explicit menu
   entry.
2. Breaking change posture — clean break at 0.8.0 with a migration table and no
   compat shim? (Recommended: yes.)
   RESOLVED 2026-06-03: YES. Clean break at 0.8.0, migration table in release
   notes, no compatibility shim.
3. Remove the unkeyed random-offset entirely; randomization only via keyed
   SipHash, no security-theater path? (Recommended: yes.)
   RESOLVED 2026-06-03: YES. Unkeyed random-offset removed entirely; `__random_seed`
   deleted; randomization only via keyed SipHash. No weak-decorrelation knob.
   `HT_SEED_RANDOM` means "generate a key for the keyed hash" and is a no-op or
   error with an unkeyed hash (see decision 6 for the failure policy).
4. Keyed default — SipHash-1-3 (faster, modern) vs SipHash-2-4 (conservative)?
   (Recommended: 1-3.)
   RESOLVED 2026-06-03: SipHash-1-3 is the default; SipHash-2-4 is also exposed
   as an alternate. Both vendored from the same CC0 reference. `ht_hash_siphash`
   is 1-3; a `ht_hash_siphash24` entry provides 2-4.
5. Bulk hash — wyhash (tiny, public domain, no SIMD) vs xxh3 (faster, larger)?
   (Recommended: wyhash.)
   RESOLVED 2026-06-03: wyhash. Vendored (public domain), portable C, no SIMD
   dependency. `ht_hash_bulk` is wyhash.
6. Entropy-failure policy — `ht_create` returns NULL when a keyed hash was
   requested and the CSPRNG fails, with an opt-in graceful-degradation flag?
   (Recommended: yes.)
   RESOLVED 2026-06-03: YES. Default is fail-closed — `ht_create` returns NULL
   when a keyed hash is requested and the CSPRNG fails. An opt-in
   `HT_SEED_BEST_EFFORT` flag falls back to the unkeyed default hash and signals
   the downgrade. No silent-weak path.
7. Wrapper shipping order — which of `ht_strptr`, `ht_strblob`, `ht_strset`,
   `ht_u64ptr`, `ht_u64blob` land first. (Evidence ranks: `ht_strblob` and the
   `ht_u64*` pair have the strongest support; `ht_strset` the narrowest.)
   RESOLVED 2026-06-03: order is NOT load-bearing — clean break means no
   cross-version compatibility window, and wrappers are independent leaf modules.
   The only constraint is build-convenience: introduce `ht_strblob`'s binary-value
   machinery before `ht_u64blob` reuses it. Default to `ht_strblob` ->
   `ht_u64ptr`/`ht_u64blob` -> `ht_strptr` -> `ht_strset`. The substantive ask is
   not order but runnable demos — see §12.

8. Base-layer key length — how does `len` reach the hash, given the engine
   currently assumes NUL-terminated keys?
   RESOLVED 2026-06-04: Option B — a `size_t (*keylen)(const void *key)` callback
   supplied at `ht_create`, used solely to feed the hash. `ht_insert`/`ht_get`/
   `ht_remove`, `keyeq`, `key_copy`, `key_free` keep their current signatures; no
   per-entry length is stored. Covers strings (`strlen`) and every fixed-size key
   (constant `sizeof`) — the full set of audited shapes. Naked variable-length
   binary keys must encode their own length (e.g. a flexible-array
   `{size_t len; char data[];}` key with a trivial keylen), which is an
   application-layer concern, not an engine change. Explicit-length base entry
   points may be added additively later if a real consumer needs them.

## 11. Suggested sequencing

Grouped by technical scope, not by release ceremony:

- Interface and core: the new `ht_hash` signature, fixed 64-bit output, the
  table-owned key buffer, deletion of `__random_seed`. This is the load-bearing
  change everything else builds on.
- Entropy: `ht_random_bytes` and its one-TU platform dispatch.
- Algorithms: `ht_hash_int`, then the vendored `ht_hash_bulk` and
  `ht_hash_siphash`.
- Wrappers: land `ht_strblob` (with the seed-ignored fix) and the `ht_u64*`
  pair first by evidence strength, then `ht_strptr` and `ht_strset`.

Cross-references: the four-part research and the lush-shape audit that ground
every claim here are recorded in the project memory under the pluggable-hash
vision, with the upstream roadmap (`docs/libhashtable-real-world-use.md` §6–§9)
supplying the wrapper-gap evidence.

## 12. Worked examples (demos)

A first-class deliverable, not an afterthought: an `examples/` directory with one
minimal, self-contained, runnable program per capability (~30 lines each), built
behind a meson option. These double as documentation and as the primary way a
prospective consumer sees the library work. Domains are deliberately GENERIC, not
modeled on any one downstream, to reinforce general-purpose positioning.

- `examples/wordcount.c` — string -> int (`ht_strint`): the classic associative
  map; word-frequency over stdin.
- `examples/entity_index.c` — uint64 -> pointer (`ht_u64ptr`): the FLAGSHIP demo
  of the new architecture. A `uint64_t` handle keys a record directly, with no
  stringification and the integer finalizer chosen automatically — the exact
  pattern a real consumer previously had to hand-roll. Best single proof the
  pluggable, type-matched design pays off.
- `examples/record_store.c` — string -> binary blob (`ht_strblob`): values with
  embedded NULs, demonstrating binary-safe storage.
- `examples/request_router.c` — keyed SipHash over untrusted string keys: the
  opt-in flooding-resistance story, contrasted with the FNV-1a default.

Each demo carries a short comment stating the shape it represents and why its
default algorithm fits, so the type-matching rationale is visible in the code.

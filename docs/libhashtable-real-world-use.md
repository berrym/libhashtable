# libhashtable in the Real World: A Field Report from lush

## 1. Why this document exists

libhashtable was authored largely in isolation: a small, MIT-licensed C99
hash table with a generic core (`ht_t`) plus four typed wrappers
(`ht_strstr`, `ht_strint`, `ht_strfloat`, `ht_strdouble`). It has good
test coverage of its own primitives but, until lush adopted it, no
sustained downstream use to discipline the API.

The lush shell — a bash/zsh-compatible interactive shell with an
embedded line editor (LLE) — has now vendored libhashtable for several
years and uses it across roughly a dozen subsystems. Lush has exercised
the library hard enough to (a) force a new typed wrapper into existence
locally (`ht_strblob`), (b) build a substantial higher-level wrapper
layer (`include/lle/hashtable.h`, 419 lines of API + 1102 lines of
implementation), and (c) carry a documented catalogue of "we worked
around this because libhashtable didn't have X" comments throughout the
code.

This document is the result of a multi-agent audit of lush against
libhashtable. Its purpose is concrete: to give the library's maintainer
and future contributors a single, evidence-anchored view of what real
production use revealed about the API surface, what is durable, what is
clearly missing, and which roadmap items would have the largest impact
on the principal downstream.

## 2. The library at a glance

libhashtable exposes two layers:

**Generic core.** `ht_t` is an opaque handle constructed by:

```c
ht_t *ht_create(ht_hash hash, ht_keyeq keyeq,
                const ht_callbacks_t *callbacks, unsigned int flags);
```

`ht_callbacks_t` is a struct of four function pointers
(`key_copy`/`key_free`/`val_copy`/`val_free`). The caller supplies the
hash function, the equality predicate, and the lifecycle callbacks; the
table owns whatever the callbacks copy in. The generic API is
parameterless: `ht_callbacks_t` has no `user_data` field.

**Typed wrappers.** Four small `.c` files build on the generic core to
provide string-keyed convenience tables: `ht_strstr` (string→string),
`ht_strint`, `ht_strfloat`, `ht_strdouble`. Each calls `strdup`/`free`
on the string key and uses the FNV-1a string hash from `ht_fnv1a.c`,
with optional `HT_STR_CASECMP` (case-insensitive) and `HT_SEED_RANDOM`
(per-instance random seed) flags.

The generic core's hash width is selected at compile time via
`CPU_32_BIT`/`CPU_64_BIT` macros driven by a `cpu_family()` ladder in
`meson.build`. Two long-lived branches (`32bit`, `64bit`) exist in
addition to `master` purely to hard-code one width or the other; see
§10.

There is no `ht_size`, no `ht_clear`, no `ht_contains`, no return
status on `ht_*_insert` or `ht_*_remove`, no defined contract for
mutating a table during enumeration, no per-table statistics accessor,
and no `user_data` channel for callbacks. Each of these absences is
worked around at least once in lush.

## 3. How lush actually uses libhashtable

### 3.1 Alias subsystem

The shell alias table is the cleanest user of `ht_strstr` in lush.

- **Table**: global `aliases` — `ht_strstr_t *`, string alias name →
  string expansion, created with
  `HT_STR_CASECMP | HT_SEED_RANDOM` at
  `src/builtins/alias.c:40`, destroyed at `src/builtins/alias.c:60`,
  re-created in place by `unalias -a` at
  `src/builtins/alias.c:893-898`.
- **Enumerator**: file-scope `aliases_e` used by `print_aliases()` at
  `src/builtins/alias.c:111-120`.

All alias names are NFC-canonicalized through
`lush_ident_canonicalize_alloc` before any table operation
(`src/builtins/alias.c:83, :145, :164, :242`), because libhashtable
hashes bytewise and `HT_STR_CASECMP` folds only ASCII. `set_alias`
performs a re-lookup after `ht_strstr_insert` to synthesize a success
status the API does not provide (`src/builtins/alias.c:149-152`).

### 3.2 Symbol table (variables / scopes)

The symtable is by far the heaviest `ht_strstr` user.

- **One `ht_strstr_t` per scope frame**: the global scope is created at
  `src/symtable.c:253`; pushed frames at `src/symtable.c:396`
  (`SCOPE_FUNCTION`/`LOOP`/`SUBSHELL`/`CONDITIONAL`) and at
  `src/symtable.c:482` (`SCOPE_LEXICAL` for typed-function closures).
- **One `ht_strstr_t` per associative array**: `array_value_t::assoc_map`
  created at `src/symtable.c:2473`.

The variable value carries four logical fields
(`value | type | flags | scope_level`) flattened into a pipe-delimited
string and round-tripped through `serialize_variable` /
`deserialize_variable` (`src/symtable.c:98-207`). Array-valued
variables encode their `array_value_t*` as `%p` hex text inside the
same value slot and recover it with `sscanf("%p")`
(`src/symtable.c:199-203, :3107-3116`). Because the table does not
know it holds pointer-strings, `free_arrays_in_scope` walks the table
and frees the pointed-to `array_value_t` blocks before each
`ht_strstr_destroy` (`src/symtable.c:531-549`).

Variable unset is implemented as a sentinel re-insert
(`SYMVAR_UNSET`) rather than `ht_strstr_remove`, so that an inner
scope's unset masks rather than reveals an outer-scope binding
(`src/symtable.c:976-1006, :353`).

### 3.3 Builtins: `hash` and `declare`

- **Table**: `command_hash` (`ht_strstr_t *`, declared at
  `include/builtins.h:597`, defined at
  `src/builtins/builtins.c:52`). Holds the POSIX `hash` builtin's
  utility→absolute-path mapping AND doubles as the executor's positive
  PATH-resolution cache. Created with
  `HT_STR_CASECMP | HT_SEED_RANDOM` at
  `src/builtins/builtins.c:471`; destroyed at
  `src/builtins/builtins.c:477`; destroy-and-recreated in place by
  `hash -r` at `src/builtins/bin_hash.c:32-37`.

A separate, fixed-size 32-entry FIFO struct array with per-entry TTL
(`path_neg_cache`) sits next to `command_hash` to track failed PATH
lookups (`src/builtins/builtins.c:75-148`); it does not use
libhashtable at all because the library has no TTL primitive.

`bin_declare` does not own any hashtable; it only enumerates the
symtable's `array_value_t::assoc_map` for `declare -p` listings
(`src/builtins/bin_declare.c:43-72`).

### 3.4 Executor

The ~18 kLOC executor contains exactly **one** libhashtable call site:
a write into `command_hash` from the external-command path
(`src/executor.c:8702-8708`). Shell function definitions are held in a
singly-linked list rooted at `executor->functions` and walked linearly
(`src/executor.c:9817-9840, :9856-9909, :15526-15543`). Hook dispatch
reuses the same linked-list walk (`src/executor.c:18338`). A long
comment at `src/executor.c:18494-18506` explicitly documents that the
unspecified iteration order of libhashtable is why
`typed_fn_dup_array` walks the symtable's parallel insertion-order
array rather than enumerating the underlying `assoc_map`.

### 3.5 Autoload

The autoload registry **deliberately does not** use libhashtable. The
source comment at `src/autoload.c:42-47` reads, in part:

> The set of autoloadable names a script declares is small in practice
> (oh-my-zsh's plugin loader rarely tops a few dozen); a list keeps the
> implementation obvious and avoids dragging in a hash table just for
> this. Replace with the strstr hashtable if profiling justifies it.

Registry primitives mimic the API surface a libhashtable wrapper would
expose so the upgrade path is one-to-one
(`src/autoload.c:56-116`).

### 3.6 Config / config_registry / TOML

The config subsystem **does not use libhashtable at all** — verified by
exhaustive grep across `config.c`, `config.h`, `config_registry.c`,
`config_registry.h`, `toml_parser.c`, `toml_parser.h`. Storage is
fixed-size C arrays
(`registered_section_t sections[CREG_SECTION_MAX]` at
`src/config_registry.c:99`) with O(N) strcmp lookups
(`src/config_registry.c:314`). This is a deliberate design choice for
deterministic memory footprint, not a libhashtable shortcoming.

### 3.7 Debug / error subsystems

Audited and confirmed clean of libhashtable. The only ostensibly
related field, `void *error_suppression_table` at
`include/lle/error_handling.h:442`, is forward-declared but never
assigned, initialized, or read anywhere in the tree — a stub from the
LLE specification.

### 3.8 Display layer (non-LLE)

The 12 files under `src/display/` and `include/display/` contain zero
libhashtable references. The display layers cache rendering state in
plain structs and fixed-size arrays
(`command_layer.h:241 highlight_regions[]`).

### 3.9 LLE completion

The completion subsystem owns **no** hashtables of its own. It is a
pure consumer:

- `src/lle/completion/completion_sources.c:126-142` enumerates the
  alias table read-only (`extern ht_strstr_t *aliases;` from
  `include/alias.h:23`) and prefix-filters via `nfc_prefix_match`.
- `src/lle/completion/ssh_hosts.c` uses a grow-on-demand
  `ssh_host_t[]` array (`ssh_hosts.c:72-94`) with linear-search find
  (`ssh_hosts.c:145-158`) — chosen because the prefix-match iteration
  has to walk every entry anyway and sources have a priority field
  that array order encodes naturally.

### 3.10 LLE syntax highlighting

Also **does not use libhashtable**. Two file-static, fixed-capacity
(128 slots), direct-mapped, evict-on-collision caches with 30-second
TTL stand in for what would otherwise be candidate consumers
(`src/lle/display/syntax_highlighting.c:130-203, :510-615,
:720-870`). The hash function is djb2; keyword and builtin sets are
NULL-terminated `const char *[]` arrays with length-prefiltered linear
scan (`syntax_highlighting.c:209-217, :225-249, :256-281`).

### 3.11 LLE keybinding

The keybinding manager is the **only** production consumer of the LLE
wrapper layer (`lle_strstr_hashtable_t`). It maps GNU Readline-style
key strings (e.g. `"C-x"`, `"M-f"`, `"UP"`) to
`lle_keybinding_entry_t *`, which it smuggles through the string-only
table by `snprintf("%p", ptr)` on insert and `sscanf("%p")` on lookup
(`src/lle/keybinding/keybinding.c:539-546, :593-600, :622-628,
:698-700, :1131-1140, :1209-1219`). The author flags this as
"hackish but works with strstr hashtable" in an inline comment.

### 3.12 LLE history index

Uses the **generic** `ht_t` API with custom callbacks for `uint64_t`
keys to `lle_history_entry_t *` values
(`src/lle/history/history_index.c:32-98`). Hash is Thomas Wang's
64-bit mix; key copy pool-allocates a `sizeof(uint64_t)` slot;
value-side callbacks are identity/no-op because the history core owns
the entries.

### 3.13 LLE widget system

Uses the generic `ht_t` directly with `callbacks=NULL` (raw pointers,
no copy/free) as an index over an externally owned singly-linked list
of widgets (`src/lle/widget/widget_system.c:78, :147, :177, :198,
:243`). The hashtable owns nothing; the parallel list owns lifetime.
This is necessary because libhashtable provides no insertion-order
iteration.

### 3.14 LLE render cache

The only consumer of `ht_strblob_t` in lush.
`src/lle/display/render_cache.c:359` creates the table; entries are
serialized `lle_cached_entry_t` blobs (ASCII metadata header +
arbitrary binary payload). The table is destroyed and recreated to
clear (`render_cache.c:606-614`) because no `ht_clear` exists. A
shadow LRU linked list (`render_cache.c:160-301`) lives parallel to
the table because libhashtable provides no iteration order or
eviction hooks.

## 4. Creative patterns worth highlighting

### 4.1 Pointer-as-hex-string smuggling

Repeated in two places to store opaque pointers through string-only
tables.

```c
// src/symtable.c:3107-3116 — array_value_t* through ht_strstr
char buf[32];
snprintf(buf, sizeof(buf), "%p", (void *)array);
/* later, on read: */
sscanf(value, "%p", (void **)&array);
```

```c
// src/lle/keybinding/keybinding.c:539-546 — entry* through ht_strstr
char buf[32];
snprintf(buf, sizeof(buf), "%p", (void *)entry);
lle_strstr_hashtable_insert(mgr->bindings, key, buf);
/* later: */
lle_keybinding_entry_t *entry;
sscanf(value_str, "%p", (void **)&entry);
```

Both pay an `snprintf`/`sscanf` per access purely because no
`ht_strptr` variant exists.

### 4.2 Serialized metadata in string values

Variables flatten four fields into one `|`-delimited string:

```c
// src/symtable.c:98-207
snprintf(buf, n, "%s|%d|%u|%d", value, type, flags, scope_level);
/* on read: tokenize on '|', atoi the trailing fields */
```

Every variable access pays a `snprintf` + `strdup` + `atoi` tax.

### 4.3 Length-prefixed blob with ASCII header

The render cache packs a struct + arbitrary binary payload into a
single `ht_strblob` value:

```c
// src/lle/display/render_cache.c:78-88 (paraphrased)
snprintf(hdr, n, "%zu:%llu:%llu:%u:%d|", data_size, ts, last_access,
         access_count, valid);
memcpy(buf, hdr, hdr_len);
memcpy(buf + hdr_len, entry->data, data_size);  /* may contain NUL */
```

On read, `memchr(bytes, '|', size)` finds the header boundary
(`render_cache.c:126`). This pattern only works because the value side
is binary-safe — which is exactly why `ht_strblob` had to be invented
(see §7).

### 4.4 Composite string keys

When the key is logically a tuple, lush concatenates with a separator:

```c
// src/config_registry.c:540
snprintf(full_key, sizeof(full_key), "%s.%s", section, key);
```

The render cache does the same trick to use a `uint64_t` as a key:

```c
// src/lle/display/render_cache.c:451-453
char key_str[32];
snprintf(key_str, sizeof(key_str), "%" PRIu64, key);
ht_strblob_insert(cache->cache_table, key_str, blob, blob_len);
```

### 4.5 Case-insensitive lookup

Three subsystems opt in to `HT_STR_CASECMP`: aliases
(`src/builtins/alias.c:40`), `command_hash`
(`src/builtins/builtins.c:471`), and the keybinding wrapper. In the
alias and `command_hash` cases this is a deliberate deviation from
POSIX (bash is case-sensitive on these); the executor inherits the
behavior of `command_hash` indirectly through its single write site.

### 4.6 Scope-stack pattern

Per-scope hashtables linked by a parent pointer give name shadowing
for free; pushing a scope allocates a new `ht_strstr_t`, popping frees
it (`src/symtable.c:344-362, :376-420, :551-580`). The
`SCOPE_LEXICAL` variant carries two parent pointers — `parent` for the
lookup chain (captured at closure creation) and `dynamic_caller` for
LIFO pop — supporting both POSIX dynamic scoping and typed-function
closure semantics in one machinery.

### 4.7 Shadow data structures parallel to the table

Multiple subsystems maintain a sidecar list because libhashtable lacks
ordering and eviction hooks:

- `array->assoc_insertion_order` (`include/symtable.h:88-104`, used at
  `src/symtable.c:2913-2935`) — zsh/lush-mode insertion-order
  iteration of associative arrays.
- `lle_display_cache_policy_t` LRU linked list
  (`render_cache.c:160-301`) — eviction policy.
- `widget_registry->widget_list` (`widget_system.c:78-127`) —
  ownership and ordered destruction.

### 4.8 Destroy-and-recreate as bulk clear

`unalias -a` (`src/builtins/alias.c:893-898`), `hash -r`
(`src/builtins/bin_hash.c:32-37`), and the render cache invalidate-all
(`src/lle/display/render_cache.c:606-614`) all teardown and rebuild
their table rather than iterating to remove because (a) there is no
`ht_clear`, and (b) mutation during enumeration is not defined.

### 4.9 Round-trip verification after insert

```c
// src/builtins/alias.c:149-152
ht_strstr_insert(aliases, key, value);
return lookup_alias(name) != NULL;  /* synthesize success */
```

Compensates for `ht_strstr_insert` having a `void` return.

### 4.10 NFC normalization at every table boundary

Because libhashtable hashes bytewise, every Unicode-identifier consumer
runs `lush_ident_canonicalize_alloc` on the key before insert / get /
remove (alias: `src/builtins/alias.c:83, :145, :164, :242`; symtable:
`src/symtable.c:669, :776, :854, :984, :3090, :3135, :2695`). An ASCII
fast-path keeps the common case allocation-free.

## 5. The lush wrapper layer

`include/lle/hashtable.h` (419 lines) and
`src/lle/core/hashtable.c` (1102 lines) build a higher-level facade
the keybinding subsystem consumes. The wrapper adds:

| Concern | Mechanism | Site |
|---|---|---|
| Lifecycle facade | `lle_hashtable_system_init/destroy`, factory, registry | `include/lle/hashtable.h:205-260` |
| Per-table thread safety | `pthread_rwlock_t` around every op | `src/lle/core/hashtable.c:506-536, :656-918` |
| O(1) size | shadow `entry_count` updated on insert/delete (with redundant pre-lookup) | `include/lle/hashtable.h:160-163`; `src/lle/core/hashtable.c:660-668, :764-773, :819-841` |
| `clear()` | snapshot-keys-then-remove, two-pass | `src/lle/core/hashtable.c:847-907` |
| `contains()` | sugar over `get(...) != NULL` | `src/lle/core/hashtable.c:797-817` |
| `remove()` status | pre-lookup to synthesize NOT_FOUND | `src/lle/core/hashtable.c:763-794` |
| Callback-style `foreach` | wraps `ht_strstr_enum_*` under read lock | `src/lle/core/hashtable.c:909-936` |
| Pool-allocator callbacks | `__thread` context smuggled through `ht_callbacks_t` | `src/lle/core/hashtable.c:28, :57-172` |
| Per-table metrics | counters + timings updated inside the lock | `include/lle/hashtable.h:120-147` |
| Registry of live tables | mutex-protected dynamic array | `src/lle/core/hashtable.c:245-341` |

Three structural problems in this wrapper are worth calling out, with
honest weight:

1. **The pooled callback path is dead for `ht_strstr`.** The comment at
   `src/lle/core/hashtable.c:487-488` admits "libhashtable callbacks
   are set during creation, not after. For now, memory pool
   integration will be added in a future phase." `ht_strstr_create`
   takes no callbacks, so the entire pooled callback infrastructure
   (`key_copy_pooled` etc.) can only fire through
   `lle_hashtable_factory_create_generic`. The use_memory_pool config
   flag is silently ignored for the strstr path.

2. **Name collision in the API.** `include/lle/performance.h:55-56`
   contains `typedef struct ht ht_t; typedef ht_t lle_hashtable_t;`
   making `lle_hashtable_t` a thin alias for the upstream type. The
   wrapper structs are `lle_strstr_hashtable_t` /
   `lle_generic_hashtable_t`. `include/lle/widget_system.h:111`
   declares `lle_hashtable_t *widgets`; the implementation at
   `src/lle/widget/widget_system.c:78` populates it with raw
   `ht_create` and never touches the wrapper. The name suggests the
   wrapper; the storage is the raw library.

3. **The `lle_concurrent_hashtable_t` and `lle_monitored_hashtable_t`
   forward declarations at `include/lle/hashtable.h:48, :52` are never
   defined.** Dead surface.

The wrapper contains a stale comment block
(`src/lle/core/hashtable.c:620-636`) claiming "typically 90-95% success
rate" of entries surviving under contention even with external rwlock.
This claim is **no longer accurate** and contradicts lush's own
subsequent investigation. lush commit `cd15c58f` (2025-10-30) traced
the originally-observed entry loss to two bugs in lush's wrapper
itself (a metrics race condition where performance counters were
updated AFTER `pthread_rwlock_unlock`, and a separate stub in
`list_bindings` that returned an empty array), neither in libhashtable.
The fix achieved 100% concurrent insert success. lush commit `b8fb50bf`
(2026-05-29) then explicitly retracted the earlier "libhashtable
enumeration is buggy" claim ("drop false libhashtable-bug claims") and
added `lle_strstr_hashtable_foreach()` that successfully uses the
enumeration. The comment at 620-636 and the parallel comment at
830-832 ("doesn't correctly count all entries in collision chains")
were missed in the retraction sweep and should be removed the next
time the file is touched. libhashtable's chain code in `src/ht.c` is
single-threaded-correct (verified by direct read of all 534 lines on
2026-05-31); external locking IS sufficient.

## 6. Where lush had to be creative — limitations discovered

Consolidated, deduplicated, ranked by how often they bite.

### 6.1 No O(1) size / count

Every subsystem that needs to know its table's cardinality maintains a
shadow counter and pays a redundant `get` per insert / delete.

- `lle_strstr_hashtable.entry_count` — get-before-insert
  (`src/lle/core/hashtable.c:660-668`), get-before-delete
  (`src/lle/core/hashtable.c:764-773`).
- `array_value_t.count` — same pattern at `src/symtable.c:2715,
  :2854`.
- `lle_history_index` size returns 0; comment tells callers to track
  it themselves.
- `symtable_count_global_vars()` is a stub returning 0
  (`src/symtable.c:1905-1908`).

### 6.2 No `ht_clear`

Three production workarounds:

- Snapshot-then-remove (`src/lle/core/hashtable.c:847-907`) —
  strdup's every key into a heap buffer, terminates the enumerator,
  re-loops with `ht_strstr_remove`.
- Destroy-and-recreate
  (`src/builtins/alias.c:893-898`,
  `src/builtins/bin_hash.c:32-37`,
  `src/lle/display/render_cache.c:606-614`).
- Documented no-op (`src/lle/history/history_index.c:237-248`).

### 6.3 No insert / remove status

`ht_*_insert` has a void return; `ht_*_remove` is void as well. Lush
synthesizes a status with a paired `get` in five files (alias,
symtable, builtins, LLE wrapper, widget system).

### 6.4 No `contains`

The `get(...) != NULL` idiom is used everywhere. It conflates "absent"
with "present-but-NULL-value" — survivable today because the symtable
deliberately substitutes `""` for NULL on insert
(`src/symtable.c:2746`).

### 6.5 No safe mutation during enumeration

Standard "collect-then-delete" workaround at
`src/lle/core/hashtable.c:863-893`. The symtable's
`free_arrays_in_scope` is read-only during enum and defers actual
table teardown to `ht_strstr_destroy`
(`src/symtable.c:531-549, :541-547, :1299-1344`).

### 6.6 No iteration order guarantee

Combined with `HT_SEED_RANDOM`, this means `$env`/`alias` listing
order changes across runs. The symtable maintains
`assoc_insertion_order` (`src/symtable.c:2864-2876, :2919-2922`) for
zsh/lush-mode iteration; bash mode accepts bucket order; alias
listing accepts bucket order; `bin_declare` does the same dance for
`declare -p`.

### 6.7 No `user_data` in `ht_callbacks_t`

The LLE wrapper passes its memory-pool context through a `__thread`
static (`src/lle/core/hashtable.c:28, :57-172`). This is fragile
under nested or cross-table use and is the root cause that the
strstr pooled path is dead — there is no way to bind a pool to
`ht_strstr_create` because that constructor accepts no callbacks at
all.

### 6.8 No primitive-keyed wrappers

- `uint64_t` keys: `src/lle/history/history_index.c:32-98` hand-rolls
  Thomas Wang hash + eq + pool-allocating copy/free.
- Pointer values through string tables: see §4.1.

### 6.9 No binary-safe value storage in `ht_strstr`

`render_cache` was bitten by issue #49 — embedded NULs silently
truncated. Lush's response was to add `ht_strblob` locally; see §7.

### 6.10 No per-table statistics

The LLE wrapper's metrics struct
(`include/lle/hashtable.h:120-147`) reserves fields for `collisions`,
`rehash_operations`, `load_factor`, `current_capacity`,
`memory_usage_bytes` — the wrapper cannot populate any of them
because libhashtable exposes no accessor.

### 6.11 No TTL / eviction hooks

`command_hash` re-validates each cache hit with `access(X_OK)` and
lazily removes the stale entry
(`src/builtins/builtins.c:508-516`). The negative-cache is a separate
fixed struct array because libhashtable has no per-entry timestamp.
The syntax highlighter rolled an entirely separate cache type
(`syntax_highlighting.c:130-203`).

### 6.12 No replacement notification

`ht_strstr_insert` over an existing key silently `strdup`-replaces
the value; the old value is unreachable to the caller. Not currently
causing bugs but blocks audit/old-value notifications and atomic
upsert use cases.

### 6.13 No "set" / presence-only variant

The autoload registry uses a linked list partly because a `ht_strstr`
"set" would waste a value slot
(`src/autoload.c:42-47, :49-116`).

### 6.14 No tunable load factor / capacity / growth

`src/libhashtable/ht.c:15-21` hardcodes `INITIAL_BUCKETS=16`,
`MAX_LOAD_FACTOR=0.75`, `GROWTH_FACTOR=2`. The LLE wrapper's config
fields (`hash_function`, `key_equality`, `max_load_factor`,
`growth_factor`, `max_capacity`, `debug_mode`, `initial_capacity`)
are stored but never read. `ht_create`'s capacity argument is
silently ignored: `src/libhashtable/ht.c:260` always sets
`ht->capacity = INITIAL_BUCKETS`.

### 6.15 Thread safety

Documented at `src/lle/core/hashtable.c:620-636`: external
`pthread_rwlock_t` is insufficient under high contention; the
collision-handling linked lists are not internally thread-safe.

## 7. `ht_strblob`: a case study in extension

`ht_strblob` is lush's sixth typed wrapper, added locally as
`src/libhashtable/ht_strblob.c` (192 LOC) with prototypes inlined into
the vendored `include/libhashtable/ht.h`. The motivation, recorded in
a comment at `src/lle/display/render_cache.c:14-16, :352-359`, is GitHub
issue #49: `ht_strstr`'s `strdup`/`strlen` path silently truncates
values at the first `0x00` byte. The render cache stores serialized
cache entries that contain arbitrary bytes including NULs, so
`ht_strstr` was a correctness bug.

**Internal representation** — private to the implementation file
(`src/libhashtable/ht_strblob.c:27-30`):

```c
typedef struct {
    void *data;
    size_t size;
} ht_blob_t;
```

Callers never see `ht_blob_t`; the API surface uses
`(const void *data, size_t size)` pairs.

**Constructor** wires `ht_callbacks_t` exactly like `ht_strstr_create`
but swaps the value half (`src/libhashtable/ht_strblob.c:76-89`):

```c
ht_strblob_t *ht_strblob_create(unsigned int flags) {
    ht_hash hash = fnv1a_hash_str;
    ht_keyeq keyeq = str_eq;
    const ht_callbacks_t callbacks = {
        (void *(*)(const void *))strdup, (void (*)(const void *))free,
        (void *(*)(const void *))blob_copy, (void (*)(const void *))blob_free};
    if (flags & HT_STR_CASECMP) {
        hash = fnv1a_hash_str_casecmp;
        keyeq = str_caseeq;
    }
    return (ht_strblob_t *)ht_create(hash, keyeq, &callbacks, flags);
}
```

`blob_copy` allocates both a new `ht_blob_t` header and a separate
`malloc` for the bytes, then `memcpy`s; zero-size blobs store
`data=NULL, size=0`. `blob_free` reverses the two allocations.

**Insert** constructs a stack `ht_blob_t` and hands its address to the
generic layer, which invokes `val_copy=blob_copy` to make the heap
copy (`src/libhashtable/ht_strblob.c:108-112`):

```c
void ht_strblob_insert(ht_strblob_t *ht, const char *key,
                       const void *data, size_t size) {
    ht_blob_t blob = {(void *)data, size};
    ht_insert((ht_t *)ht, (void *)key, &blob);
}
```

The caller retains ownership of its `data` buffer and may free it
immediately after `ht_strblob_insert` returns — confirmed at
`src/lle/display/render_cache.c:481-487`, which calls
`lle_pool_free(serialized)` on the next line.

**Get** returns an aliased pointer into the table's internal storage
with the byte count via an out-parameter
(`src/libhashtable/ht_strblob.c:136-149`); the contract documented in
the lush-vendored header at `include/libhashtable/ht.h:399-412` is that
the pointer is valid until the entry is removed or the table
destroyed.

**Enumeration** mirrors get: `ht_strblob_enum_next` writes `key`,
`data`, and `size` through three out-parameters, any of which may be
NULL to skip (`src/libhashtable/ht_strblob.c:172-186`).

Style parity with `ht_strstr.c` is high; the structural difference is
the private `ht_blob_t` header and the `(data, size)` signature on
insert/get/enum_next. Upstreaming requires no core change to `ht.c`.

The only consumer in lush is `src/lle/display/render_cache.c`. Lush did
**not** build an `lle_strblob` wrapper — the inline comment at
`render_cache.c:356-358` explicitly says "an lle_strblob wrapper can
be added later if a second consumer needs memory-context or metrics
integration." Today render_cache talks to `ht_strblob` directly.

## 8. Vendor drift: how lush's copy diverged

Lush vendors 7 `.c` files and 1 header at
`/home/mberry/Lab/c/lush/src/libhashtable/` and
`/home/mberry/Lab/c/lush/include/libhashtable/`. The functional delta
versus upstream's `64bit` branch is small.

| File | Real change | Cosmetic change |
|---|---|---|
| `ht.h` | New `ht_strblob_t` type + 8 prototypes; `#include <stddef.h>` for `size_t` | Full Doxygen blocks on every prototype; license header reformatted |
| `ht.c` | `MAX_CAPACITY` cast: `(1 << 31)` → `((size_t)1 << 31)`; `__ht_passthrough_destroy` parameter annotated `__attribute__((unused))`; `#include <sys/types.h>` added | All `/* */` comments converted to Doxygen |
| `ht_fnv1a.c` | `#include <strings.h>` plus a redundant inline `int strcasecmp(...)` forward declaration for stricter libcs | Doxygen-ification |
| `ht_strdouble.c`, `ht_strfloat.c`, `ht_strint.c`, `ht_strstr.c` | None | Doxygen-ification; lush spells "libhashtable" correctly where upstream `ht_strint.c:3` has a typo `libashtable` |
| `ht_strblob.c` | **Entirely new file (192 LOC)** | n/a |
| `meson.build` | Not vendored — files are compiled directly by lush's top-level `meson.build:172-178, :350, :881, :1304, :1326` | n/a |

The `MAX_CAPACITY` change is a real undefined-behavior fix (signed-int
shift into the sign bit, C11 6.5.7p4); lush commit `7fdc444c`
documented that UBSan trips it within thousands of inputs during
executor fuzz runs. Upstream's `64bit` branch has the dropped
`ssize_t` cast at `__ht_rehash` already done independently
(`upstream src/ht.c:181`).

License headers diverge in wording ("MIT (relicensed for Lush
integration)") but semantically remain MIT both sides.

## 9. Recommendations for libhashtable upstream

These are derived directly from the workarounds catalogued above.
Priority reflects how often the gap is hit in lush.

### 9.1 High-priority additions

**`ht_strblob` typed wrapper.** Copy verbatim from
`lush/src/libhashtable/ht_strblob.c`. Adds eight functions plus an
opaque `ht_strblob_t`, fixes a real silent-data-loss bug, follows the
established typed-wrapper idiom exactly, requires no changes to
`ht.c`. One downstream is already shipping it in a public-looking
header; renaming would break that downstream.

```c
typedef struct ht_strblob ht_strblob_t;
ht_strblob_t *ht_strblob_create(unsigned int flags);
void          ht_strblob_destroy(ht_strblob_t *);
void          ht_strblob_insert(ht_strblob_t *, const char *key,
                                const void *data, size_t size);
void          ht_strblob_remove(ht_strblob_t *, const char *key);
const void   *ht_strblob_get(ht_strblob_t *, const char *key,
                             size_t *out_size);
ht_enum_t    *ht_strblob_enum_create(ht_strblob_t *);
bool          ht_strblob_enum_next(ht_enum_t *, const char **key,
                                   const void **data, size_t *size);
void          ht_strblob_enum_destroy(ht_enum_t *);
```

Document in `ht.h` the alias-lifetime rule for `ht_strblob_get`'s
return: invalidated by remove/destroy. Add a regression test that
round-trips a value containing `0x00`.

**`ht_size` / `ht_*_size`.** O(1) entry-count accessor. Eliminates
shadow counters in the LLE wrapper (`include/lle/hashtable.h:160-163,
:176-179`), the symtable, and the history index.

```c
size_t ht_size(const ht_t *ht);
size_t ht_strstr_size(const ht_strstr_t *ht);
/* ... matching per-typed-wrapper */
```

**Insert / remove with status.** Eliminates get-before-write probes
in five files.

```c
typedef enum {
    HT_OK_INSERTED, HT_OK_UPDATED, HT_OK_REMOVED,
    HT_ERR_ABSENT, HT_ERR_OOM
} ht_status_t;

ht_status_t ht_insert_ex(ht_t *ht, const void *key, const void *val,
                         void **prev_val_out);
ht_status_t ht_remove_ex(ht_t *ht, const void *key);
```

Or, less invasively, change the existing void returns to `bool`
indicating "was new" / "was present."

**`user_data` channel in `ht_callbacks_t`.** Without this, allocator
integrations cannot pass their pool through. The LLE wrapper carries
a `__thread` static
(`src/lle/core/hashtable.c:28 current_memory_context`) explicitly to
work around this absence, and the strstr pooled path is dead because
`ht_strstr_create` accepts no callbacks at all.

```c
typedef struct {
    void *(*key_copy)(const void *key, void *user_data);
    void  (*key_free)(const void *key, void *user_data);
    void *(*val_copy)(const void *val, void *user_data);
    void  (*val_free)(const void *val, void *user_data);
    void  *user_data;
} ht_callbacks_t;
```

Equivalently, expose `ht_strstr_create_ex(flags, callbacks)` so typed
wrappers can be pool-backed.

**`ht_strblob` upstreaming aside**, also expose **pointer-value and
integer-key wrappers** to kill the two largest serialization hacks
(§4.1, §6.8):

```c
typedef struct ht_strptr ht_strptr_t;
ht_strptr_t *ht_strptr_create(unsigned int flags);
ht_status_t  ht_strptr_insert(ht_strptr_t *, const char *key, void *val);
void        *ht_strptr_get(ht_strptr_t *, const char *key);
ht_status_t  ht_strptr_remove(ht_strptr_t *, const char *key);

typedef struct ht_intptr ht_intptr_t;
ht_intptr_t *ht_intptr_create(unsigned int flags);
ht_status_t  ht_intptr_insert(ht_intptr_t *, uint64_t key, void *val);
void        *ht_intptr_get(ht_intptr_t *, uint64_t key);
ht_status_t  ht_intptr_remove(ht_intptr_t *, uint64_t key);
```

The integer-key wrapper should use a high-quality integer mix
(splitmix64 or xxHash3); lush's `hash_uint64` (Thomas Wang) is a
reasonable in-tree reference.

**Internal `MAX_CAPACITY` cast.** One-line UB fix:

```c
#define MAX_CAPACITY ((size_t)1 << 31)
```

Lush has carried this for two years (commit `7fdc444c`).

### 9.2 Medium-priority additions

**`ht_clear` / `ht_*_clear`.** Removes the snapshot-then-delete and
destroy-and-recreate workarounds in three subsystems.

```c
void ht_clear(ht_t *ht);
void ht_strstr_clear(ht_strstr_t *ht);
```

**`ht_contains` / `ht_*_contains`.** Sugar, but lets `NULL` become a
legitimate stored value and matches caller expectations.

```c
bool ht_contains(const ht_t *ht, const void *key);
bool ht_strstr_contains(const ht_strstr_t *ht, const char *key);
```

**`ht_foreach` callback iterator.** Eliminates the
`create/next/destroy` boilerplate duplicated at every iteration site.

```c
typedef bool (*ht_visit_fn)(const void *key, const void *val,
                            void *user);  /* return false to stop */
void ht_foreach(ht_t *ht, ht_visit_fn cb, void *user);
```

Document whether in-callback `ht_remove` on the current key is safe
(if not, add `ht_remove_if` as a paired primitive).

**`ht_stats` introspection.** Populates the fields the LLE wrapper's
metrics struct reserves but cannot fill.

```c
typedef struct {
    size_t count;
    size_t capacity;
    size_t collisions;
    size_t longest_chain;
    uint64_t rehashes;
    double load_factor;
    size_t bytes;
} ht_stats_t;
void ht_get_stats(const ht_t *ht, ht_stats_t *out);
```

**`ht_create_ex` with options.** Make `initial_capacity`,
`max_load_factor`, `growth_factor` actually do something. Currently
the `initial` parameter to `ht_create` is silently ignored
(`src/ht.c:260`).

```c
typedef struct {
    unsigned int flags;
    uint32_t     initial_capacity;
    double       max_load_factor;
    uint32_t     growth_factor;
    uint32_t     max_capacity;
    uint64_t     seed;
} ht_options_t;

ht_t *ht_create_ex(ht_hash, ht_keyeq, const ht_callbacks_t *,
                   const ht_options_t *opts);
```

**`ht_fnv1a.c` portability include.** Add `#include <strings.h>` so
`strcasecmp` is properly declared on strict libcs.

**`ht_strint.c:3` typo.** `libashtable` → `libhashtable`.

### 9.3 Low-priority / consider-and-discard

**`HT_PRESERVE_INSERTION_ORDER` flag plus ordered enumeration.** Would
let the symtable retire its parallel `assoc_insertion_order` array
(`src/symtable.c:2511-2517, :2864-2876, :2919-2922`). Cost is a
doubly-linked overlay; benefit is real but narrow — only the
zsh/lush mode of one subsystem currently needs it.

**`HT_THREAD_SAFE` flag.** Per-table mutex or rwlock, opt-in via the
existing `ht_flags_enum_t` idiom (matching `HT_STR_CASECMP`,
`HT_SEED_RANDOM`). The LLE wrapper already wraps every call in
`pthread_rwlock_*` and would benefit from removing that scaffolding
once libhashtable owns the locking. Direct review of `src/ht.c`'s
chain code on 2026-05-31 confirms it is single-threaded-correct;
external locking IS sufficient and the lush wrapper's stale comments
to the contrary at `src/lle/core/hashtable.c:620-636` and `:830-832`
are leftovers from a misdiagnosis lush itself retracted in commits
`cd15c58f` and `b8fb50bf` (see §5 footnote). The thread-safety
implementation question is therefore not about fixing the chain code
but about (a) the backend — C11 `<threads.h>` where available with
pthread fallback for macOS and older glibc, (b) compound primitives
(`ht_get_or_insert`, `ht_size_locked + ht_foreach_locked` under one
lock interval) so callers stop needing TOCTOU-prone two-call
sequences, and (c) lock granularity (per-table mutex is sufficient
for current lush usage; per-bucket or lock-free are V2 considerations).

**`ht_callbacks_ex_t` (size-aware copy/free).** A generalization of
the `ht_strblob` pattern. Low priority once `ht_strblob` itself
ships, because the typed wrapper already solves the immediate need.

**`ht_clone` / `ht_merge`.** Useful if symtable scope-stacking ever
wants inheritance, but no current code path needs them.

**`__ht_passthrough_destroy` unused-parameter annotation.** Build
hygiene only.

## 10. Branch strategy recommendation

### 10.1 Current layout

Three live branches: `master`, `32bit`, `64bit`, plus matching
`origin/*` remotes. All three diverge from the same root commit
`72d6c8b` (2024-01-26). The `32bit` and `64bit` branches were created
on the same day, four days after master diverged, with sibling "First
commit of XXbit branch" commits (`b867843`, `e55162a`).

The split is **not about pointer width**. `ht.c` does not depend on
`sizeof(void*)` anywhere. The only width-sensitive code is the FNV-1a
return type and the FNV multiplier:

- `master` selects 32-bit vs 64-bit via `#if defined(CPU_32_BIT)`,
  driven by a 15-elif `cpu_family()` ladder in `meson.build` lines 1-39.
- `64bit` is `master` with the `#if`/`#else` collapsed and only the
  64-bit half kept.
- `32bit` is the mirror image.

`git diff master 64bit --stat` shows 14 files changed, 14 insertions,
134 deletions — **all deletions, zero added functionality**.

### 10.2 Problems with the current layout

1. **Every change has to be applied three times.** The four
   `clang-format` commits in 2024-09 (`72b6758`, `25ddb94`, `920c250`,
   `978eb23`, `2e680f3`) were applied manually per branch rather than
   cherry-picked, with different SHAs and messages — concrete drift
   risk evidence.

2. **The `64bit` branch's pointer-cast fix (`346abab`) and `master`'s
   conditional-macros fix (`bf527f7`) are sibling fixes for the same
   era issue.** The fact that they are not the same commit is the
   maintenance cost.

3. **All three branches went silent after 2024-09-07.** The split is
   not being actively maintained as parallel work.

4. **The split does not represent a true 32-vs-64 portability claim.**
   It only hardcodes the FNV constant set. Lush, the principal
   downstream, only cares about 64-bit and has independently pinned
   to the `64bit` shape.

5. **`master` already behaves correctly on unknown 64-bit platforms**
   because the unset-macro case falls through to the 64-bit `#else`
   branch.

### 10.3 Recommendation: collapse to `master` only

Drive width selection via (a) automatic stdint-based detection and
(b) an explicit meson option override. Delete `32bit` and `64bit`
after one final archive tag.

Suggested header pattern for `include/ht.h`:

```c
#include <stdint.h>

#if !defined(HT_HASH_WIDTH)
#  if UINTPTR_MAX == 0xFFFFFFFFu
#    define HT_HASH_WIDTH 32
#  else
#    define HT_HASH_WIDTH 64
#  endif
#endif

#if HT_HASH_WIDTH == 32
typedef uint32_t ht_hash_t;
#  define HT_FNV1A_PRIME  0x01000193u
#  define HT_FNV1A_OFFSET 0x811C9DC5u
#else
typedef uint64_t ht_hash_t;
#  define HT_FNV1A_PRIME  0x00000100000001B3ull
#  define HT_FNV1A_OFFSET 0xCBF29CE484222325ull
#endif

typedef ht_hash_t (*ht_hash)(const void *, ht_hash_t);
```

Add a meson option:

```meson
option('hash_width', type: 'combo',
       choices: ['auto', '32', '64'], value: 'auto')
```

Translated to `-DHT_HASH_WIDTH=<n>` only when not `auto`.

### 10.4 Migration plan

Run from `/home/mberry/Lab/c/libhashtable`, `master` checked out,
working tree clean:

1. `git checkout master`
2. `git tag archive/64bit-final 64bit && git tag archive/32bit-final 32bit`
   (preserve history; tags are permanent).
3. Edit `include/ht.h` to the `HT_HASH_WIDTH` pattern; collapse the
   duplicated halves in `src/ht_fnv1a.c` into one template using
   `ht_hash_t` and `HT_FNV1A_PRIME`/`OFFSET`; shrink the
   `meson.build` ladder to the single option.
4. Build and test on x86_64 and (if available) cross-compile to i686:
   `meson setup build && meson test -C build` and
   `meson setup build32 --cross-file cross/i686.txt -Dhash_width=32 && meson test -C build32`.
5. Bump version to 0.7.0 (source-compatible but the build knob
   changed). Document `HT_HASH_WIDTH` and the meson option in the
   README.
6. `git commit -m 'Unify 32/64-bit branches into master via HT_HASH_WIDTH autodetect'`
7. `git push origin master --tags`
8. Once lush has shipped a release against the unified master:
   `git push origin --delete 32bit 64bit && git branch -D 32bit 64bit`.

Update lush's vendored copy to track unified master and drop the
width-specific patch posture; `ht_strblob.c` is additive and
unaffected by the width change.

### 10.5 Alternatives rejected

- **Option A (keep three branches)**: zero benefit, demonstrable drift
  cost, and the split does not even represent a true portability claim.
- **Option B (`#if UINTPTR_MAX` only, no meson option)**: works for
  native builds but removes the user's ability to force 32-bit hashes
  on a 64-bit host for hash-stability or test-vector reasons.
- **Option D (runtime selection)**: adds a function-pointer
  indirection on every hash for no benefit; the FNV constant set
  must be fixed at compile time for the typed wrappers to remain
  ABI-stable across translation units.

## 11. Closing assessment

### What makes libhashtable durable

The generic-core-plus-typed-wrapper layering is sound. The two-tier
API lets straightforward consumers (alias, hash builtin) stay in the
typed wrappers without ever seeing `ht_callbacks_t`, while advanced
consumers (history index, widget system) drop to the generic core with
custom callbacks. Lush has been able to write `ht_strblob` as a fifth
typed wrapper without touching the core. The MIT license, small
surface area, and absence of dependencies make vendoring trivial.

The library has also stayed out of trouble in places where it could
have over-reached: it has no opinions about ordering, eviction,
serialization, or thread safety, which is correct for a building
block.

### What is holding it back

The catalogue in §6 is the answer. The two highest-impact gaps are
**`user_data` in `ht_callbacks_t`** (every allocator-integration
downstream will pay the LLE wrapper's `__thread` tax until this is
fixed) and **insert / remove returning status** (every consumer of a
size-tracking shadow counter pays one redundant `get` per write).
Adding `ht_size`, `ht_clear`, and `ht_contains` collectively erases
hundreds of lines of wrapper code and several documented
correctness-adjacent comments in lush.

The branch sprawl in §10 is the lowest-cost, highest-clarity cleanup
available; it requires no API change.

### Prioritized roadmap

1. **Land `ht_strblob` upstream and adopt the `MAX_CAPACITY` UB fix.**
   Both are already running in production at lush. (One release.)
2. **Add `ht_size`, `ht_clear`, `ht_contains`, status-returning
   insert/remove.** All independent, all small. (One release.)
3. **Add `user_data` to `ht_callbacks_t`** (or
   `ht_strstr_create_ex` accepting callbacks). Lets the LLE wrapper
   delete its `__thread` workaround and turn the dead pooled-callback
   path back on. (One release; API addition, not change.)
4. **Add `ht_strptr` and `ht_intptr` typed wrappers.** Kills two
   classes of serialization hacks at the lush call sites. (One
   release.)
5. **Collapse the `32bit`/`64bit` branches into `master` with
   `HT_HASH_WIDTH` autodetect.** (Done in a single PR; tags preserve
   history.)
6. **Add `ht_foreach`, `ht_stats`, `ht_create_ex` with options.**
   Removes the LLE wrapper's per-table metrics dead fields and the
   `initial_capacity`-silently-ignored gotcha.
7. **Address the thread-safety story documented at
   `src/lle/core/hashtable.c:620-636`.** Either fix the
   collision-handling lost-update under contention or document it as
   a known limitation.

Items 1-2 alone retire roughly a third of the LLE wrapper's
implementation. Items 1-5 together would put the library and its
principal downstream in a place where the wrapper layer exists for
integration concerns (memory pools, metrics, error codes) rather than
to paper over missing primitives.

---

*Generated by a multi-agent audit of lush at `~/Lab/c/lush` against
libhashtable at `~/Lab/c/libhashtable`.*

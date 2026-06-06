/**
 * @file ht.h
 * @brief Generic hash table with type-matched hashing and typed wrappers.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 *
 * A generic void* core paired with type-safe wrappers and a curated menu of
 * type-matched hash algorithms. Construction is configured through option
 * structs; keyed hashing (SipHash) draws its key material from the system
 * CSPRNG.
 */

#ifndef __HT_H__
#define __HT_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque table and enumeration types. Each typed wrapper is an opaque alias
 * over the generic table; callers hold only pointers. */
typedef struct ht ht_t;
typedef struct ht_enum ht_enum_t;
typedef struct ht_strblob ht_strblob_t;
typedef struct ht_strdouble ht_strdouble_t;
typedef struct ht_strfloat ht_strfloat_t;
typedef struct ht_strint ht_strint_t;
typedef struct ht_strptr ht_strptr_t;
typedef struct ht_strset ht_strset_t;
typedef struct ht_strstr ht_strstr_t;
typedef struct ht_u64ptr ht_u64ptr_t;
typedef struct ht_u64blob ht_u64blob_t;

/**
 * @brief How a table obtains the key material passed to a keyed hash.
 */
typedef enum {
    HT_KEY_NONE = 0, ///< unkeyed hash; hashkey is NULL
    HT_KEY_RANDOM,   ///< per-table key generated from the CSPRNG
    HT_KEY_PROVIDED, ///< caller supplies the 16-byte key material
} ht_key_mode;

/* Hash values are unconditionally 64-bit. */
typedef uint64_t ht_hash_t;
#define FNV1A_PRIME (0x00000100000001B3ull)  ///< 1099511628211
#define FNV1A_OFFSET (0xCBF29CE484222325ull) ///< 14695981039346656037

/**
 * @brief A hash function over a key.
 *
 * @param key Pointer to the key bytes.
 * @param len Length of the key in bytes.
 * @param hashkey Keying material for keyed PRFs (16 bytes for SipHash), or NULL
 *                for unkeyed hashes, which ignore it.
 * @return The 64-bit hash; the table reduces it to a bucket index.
 */
typedef ht_hash_t (*ht_hash)(const void *key, size_t len, const void *hashkey);

typedef bool (*ht_keyeq)(const void *,
                         const void *);          ///< Key equality predicate
typedef size_t (*ht_keylen)(const void *);       ///< Key length in bytes
typedef void *(*ht_kcopy)(const void *, void *); ///< Key copy callback
typedef void (*ht_kfree)(const void *, void *);  ///< Key free callback
typedef void *(*ht_vcopy)(const void *, void *); ///< Value copy callback
typedef void (*ht_vfree)(const void *, void *);  ///< Value free callback

/**
 * @brief Visitor invoked for each entry by ht_foreach.
 *
 * @param key The entry key.
 * @param val The entry value.
 * @param user_data The opaque pointer passed to ht_foreach.
 */
typedef void (*ht_visit)(const void *key, const void *val, void *user_data);

/**
 * @brief Key and value lifetime callbacks.
 *
 * Each callback receives the table's user_data (from ht_options_t) as its
 * second argument. Any NULL field defaults to passthrough: the table stores the
 * pointer as given and never frees it.
 */
typedef struct {
    ht_kcopy key_copy; ///< Duplicate a key on insert
    ht_kfree key_free; ///< Release a stored key
    ht_vcopy val_copy; ///< Duplicate a value on insert
    ht_vfree val_free; ///< Release a stored value
} ht_callbacks_t;

/**
 * @brief Construction options for the generic table.
 *
 * Zero-initialized fields take defaults: passthrough callbacks, HT_KEY_NONE,
 * and the default initial capacity.
 */
typedef struct {
    ht_hash hash;             ///< required
    ht_keyeq keyeq;           ///< required
    ht_keylen keylen;         ///< required
    ht_callbacks_t callbacks; ///< zero-initialized => passthrough
    void *user_data;          ///< forwarded to each callback's second argument
    ht_key_mode key_mode;     ///< default HT_KEY_NONE
    const void *key;          ///< HT_KEY_PROVIDED: 16 bytes of key material
    bool key_best_effort; ///< HT_KEY_RANDOM: degrade vs fail on CSPRNG failure
    size_t initial_capacity; ///< 0 => default
} ht_options_t;

/**
 * @brief Construction options for the string-keyed typed wrappers.
 *
 * case_insensitive and flooding_resistant are mutually exclusive (SipHash has
 * no case-folding variant); requesting both fails the create. Zero-initialized
 * gives a case-sensitive, unkeyed FNV-1a table at the default capacity.
 */
typedef struct {
    bool case_insensitive;   ///< case-insensitive keys (FNV-1a casecmp)
    bool flooding_resistant; ///< keyed SipHash with a random per-table key
    bool best_effort;        ///< flooding_resistant: degrade vs fail
    size_t initial_capacity; ///< 0 => default
} ht_str_options_t;

/**
 * @brief Construction options for the integer-keyed (uint64) typed wrappers.
 *
 * flooding_resistant selects keyed SipHash over the key bytes for adversarial
 * keys; otherwise the integer finalizer is used. Zero-initialized gives an
 * unkeyed table at the default capacity.
 */
typedef struct {
    bool flooding_resistant; ///< keyed SipHash with a random per-table key
    bool best_effort;        ///< flooding_resistant: degrade vs fail
    size_t initial_capacity; ///< 0 => default
} ht_u64_options_t;

/* Hash algorithm menu. Each matches the ht_hash signature; pick the algorithm
 * that fits the key shape. */

/**
 * @brief FNV-1a hash of a byte string; the default for short string keys.
 * @param key Pointer to the key bytes.
 * @param len Length of the key in bytes.
 * @param hashkey Unused; FNV-1a is unkeyed.
 * @return The 64-bit hash.
 */
ht_hash_t ht_hash_fnv1a(const void *key, size_t len, const void *hashkey);

/**
 * @brief Case-insensitive FNV-1a hash of a byte string.
 * @param key Pointer to the key bytes.
 * @param len Length of the key in bytes.
 * @param hashkey Unused; FNV-1a is unkeyed.
 * @return The 64-bit hash, folding ASCII case.
 */
ht_hash_t ht_hash_fnv1a_casecmp(const void *key, size_t len,
                                const void *hashkey);

/**
 * @brief Integer finalizer (splitmix64 / Stafford Mix13); the default for
 *        integer, pointer, and identity keys.
 * @param key Pointer to the integer key bytes.
 * @param len Bytes to read (up to sizeof(uint64_t)).
 * @param hashkey Unused; this hash is unkeyed.
 * @return The 64-bit hash. A bijection on 64-bit values, so distinct keys of a
 *         given width never collide.
 */
ht_hash_t ht_hash_int(const void *key, size_t len, const void *hashkey);

/**
 * @brief Bulk hash (wyhash) for long string and binary keys.
 * @param key Pointer to the key bytes.
 * @param len Length of the key in bytes.
 * @param hashkey Unused; this hash is unkeyed.
 * @return The 64-bit hash.
 */
ht_hash_t ht_hash_bulk(const void *key, size_t len, const void *hashkey);

/**
 * @brief Keyed SipHash-1-3; the default keyed hash for untrusted keys.
 * @param key Pointer to the key bytes.
 * @param len Length of the key in bytes.
 * @param hashkey 16-byte key material; must not be NULL.
 * @return The 64-bit hash.
 */
ht_hash_t ht_hash_siphash(const void *key, size_t len, const void *hashkey);

/**
 * @brief Keyed SipHash-2-4; the conservative keyed variant.
 * @param key Pointer to the key bytes.
 * @param len Length of the key in bytes.
 * @param hashkey 16-byte key material; must not be NULL.
 * @return The 64-bit hash.
 */
ht_hash_t ht_hash_siphash24(const void *key, size_t len, const void *hashkey);

/**
 * @brief Case-sensitive string key equality.
 * @param a First key.
 * @param b Second key.
 * @return true if the NUL-terminated strings are equal.
 */
bool str_eq(const void *a, const void *b);

/**
 * @brief Case-insensitive string key equality.
 * @param a First key.
 * @param b Second key.
 * @return true if the strings are equal ignoring ASCII case.
 */
bool str_caseeq(const void *a, const void *b);

/**
 * @brief String key length.
 * @param key NUL-terminated string key.
 * @return Length in bytes, excluding the terminating NUL.
 */
size_t str_len(const void *key);

/**
 * @brief Duplicate a string key, for use as a key_copy callback.
 * @param key NUL-terminated string key.
 * @param user_data Unused.
 * @return A heap copy of the string, or NULL on allocation failure.
 */
void *str_copy(const void *key, void *user_data);

/**
 * @brief Free a string key, for use as a key_free callback.
 * @param key The stored key.
 * @param user_data Unused.
 */
void str_free(const void *key, void *user_data);

/**
 * @brief Fill a buffer with cryptographically strong random bytes.
 * @param buf Destination buffer.
 * @param len Number of bytes to write.
 * @return 0 on success, -1 on failure. On failure the buffer is unusable; a
 *         weak entropy source is never substituted.
 */
int ht_random_bytes(void *buf, size_t len);

/* Creation and destruction. A NULL options pointer to a typed wrapper selects
 * that wrapper's defaults. */

/**
 * @brief Create a generic table from an options struct.
 * @param opts Construction options; hash, keyeq, and keylen are required.
 * @return The new table, or NULL on invalid options, allocation failure, or a
 *         CSPRNG failure when HT_KEY_RANDOM was requested without best-effort.
 */
ht_t *ht_create(const ht_options_t *opts);

/**
 * @brief Destroy a generic table and free its entries.
 * @param ht The table, or NULL.
 */
void ht_destroy(ht_t *ht);

/**
 * @brief Create a string->blob table.
 * @param opts Options, or NULL for defaults.
 * @return The table, or NULL on failure or an invalid option combination.
 */
ht_strblob_t *ht_strblob_create(const ht_str_options_t *opts);

/**
 * @brief Destroy a string->blob table.
 * @param ht The table, or NULL.
 */
void ht_strblob_destroy(ht_strblob_t *ht);

/**
 * @brief Create a string->double table.
 * @param opts Options, or NULL for defaults.
 * @return The table, or NULL on failure or an invalid option combination.
 */
ht_strdouble_t *ht_strdouble_create(const ht_str_options_t *opts);

/**
 * @brief Destroy a string->double table.
 * @param ht The table, or NULL.
 */
void ht_strdouble_destroy(ht_strdouble_t *ht);

/**
 * @brief Create a string->float table.
 * @param opts Options, or NULL for defaults.
 * @return The table, or NULL on failure or an invalid option combination.
 */
ht_strfloat_t *ht_strfloat_create(const ht_str_options_t *opts);

/**
 * @brief Destroy a string->float table.
 * @param ht The table, or NULL.
 */
void ht_strfloat_destroy(ht_strfloat_t *ht);

/**
 * @brief Create a string->int table.
 * @param opts Options, or NULL for defaults.
 * @return The table, or NULL on failure or an invalid option combination.
 */
ht_strint_t *ht_strint_create(const ht_str_options_t *opts);

/**
 * @brief Destroy a string->int table.
 * @param ht The table, or NULL.
 */
void ht_strint_destroy(ht_strint_t *ht);

/**
 * @brief Create a string->pointer table storing caller-owned pointers.
 * @param opts Options, or NULL for defaults.
 * @return The table, or NULL on failure or an invalid option combination.
 */
ht_strptr_t *ht_strptr_create(const ht_str_options_t *opts);

/**
 * @brief Destroy a string->pointer table. Pointed-to objects are not freed.
 * @param ht The table, or NULL.
 */
void ht_strptr_destroy(ht_strptr_t *ht);

/**
 * @brief Create a string set (membership only).
 * @param opts Options, or NULL for defaults.
 * @return The set, or NULL on failure or an invalid option combination.
 */
ht_strset_t *ht_strset_create(const ht_str_options_t *opts);

/**
 * @brief Destroy a string set.
 * @param ht The set, or NULL.
 */
void ht_strset_destroy(ht_strset_t *ht);

/**
 * @brief Create a string->string table.
 * @param opts Options, or NULL for defaults.
 * @return The table, or NULL on failure or an invalid option combination.
 */
ht_strstr_t *ht_strstr_create(const ht_str_options_t *opts);

/**
 * @brief Destroy a string->string table.
 * @param ht The table, or NULL.
 */
void ht_strstr_destroy(ht_strstr_t *ht);

/**
 * @brief Create a uint64->pointer table storing caller-owned pointers.
 * @param opts Options, or NULL for defaults.
 * @return The table, or NULL on failure.
 */
ht_u64ptr_t *ht_u64ptr_create(const ht_u64_options_t *opts);

/**
 * @brief Destroy a uint64->pointer table. Pointed-to objects are not freed.
 * @param ht The table, or NULL.
 */
void ht_u64ptr_destroy(ht_u64ptr_t *ht);

/**
 * @brief Create a uint64->blob table.
 * @param opts Options, or NULL for defaults.
 * @return The table, or NULL on failure.
 */
ht_u64blob_t *ht_u64blob_create(const ht_u64_options_t *opts);

/**
 * @brief Destroy a uint64->blob table.
 * @param ht The table, or NULL.
 */
void ht_u64blob_destroy(ht_u64blob_t *ht);

/* Insertion and removal. Inserting an existing key replaces its value. */

/**
 * @brief Insert or replace a key/value pair.
 * @param ht The table.
 * @param key The key.
 * @param val The value.
 */
void ht_insert(ht_t *ht, const void *key, const void *val);

/**
 * @brief Remove a key if present.
 * @param ht The table.
 * @param key The key.
 */
void ht_remove(ht_t *ht, const void *key);

/**
 * @brief Insert a string key mapping to a binary value.
 * @param ht The table.
 * @param key NUL-terminated string key.
 * @param data Value bytes (may contain embedded NULs); may be NULL when size 0.
 * @param size Number of bytes at data.
 */
void ht_strblob_insert(ht_strblob_t *ht, const char *key, const void *data,
                       size_t size);

/**
 * @brief Remove a string->blob entry.
 * @param ht The table.
 * @param key The string key.
 */
void ht_strblob_remove(ht_strblob_t *ht, const char *key);

/**
 * @brief Insert a string->double entry.
 * @param ht The table.
 * @param key The string key.
 * @param val Pointer to the double value, which is copied.
 */
void ht_strdouble_insert(ht_strdouble_t *ht, const char *key,
                         const double *val);

/**
 * @brief Remove a string->double entry.
 * @param ht The table.
 * @param key The string key.
 */
void ht_strdouble_remove(ht_strdouble_t *ht, const char *key);

/**
 * @brief Insert a string->float entry.
 * @param ht The table.
 * @param key The string key.
 * @param val Pointer to the float value, which is copied.
 */
void ht_strfloat_insert(ht_strfloat_t *ht, const char *key, const float *val);

/**
 * @brief Remove a string->float entry.
 * @param ht The table.
 * @param key The string key.
 */
void ht_strfloat_remove(ht_strfloat_t *ht, const char *key);

/**
 * @brief Insert a string->int entry.
 * @param ht The table.
 * @param key The string key.
 * @param val Pointer to the int value, which is copied.
 */
void ht_strint_insert(ht_strint_t *ht, const char *key, const int *val);

/**
 * @brief Remove a string->int entry.
 * @param ht The table.
 * @param key The string key.
 */
void ht_strint_remove(ht_strint_t *ht, const char *key);

/**
 * @brief Insert a string key mapping to a caller-owned pointer.
 * @param ht The table.
 * @param key The string key.
 * @param val The pointer to store; the pointed-to object is not copied.
 */
void ht_strptr_insert(ht_strptr_t *ht, const char *key, void *val);

/**
 * @brief Remove a string->pointer entry. The pointed-to object is not freed.
 * @param ht The table.
 * @param key The string key.
 */
void ht_strptr_remove(ht_strptr_t *ht, const char *key);

/**
 * @brief Add a member to a string set.
 * @param ht The set.
 * @param key The member.
 * @return true if newly added, false if already present.
 */
bool ht_strset_add(ht_strset_t *ht, const char *key);

/**
 * @brief Remove a member from a string set.
 * @param ht The set.
 * @param key The member.
 */
void ht_strset_remove(ht_strset_t *ht, const char *key);

/**
 * @brief Insert a string->string entry.
 * @param ht The table.
 * @param key The string key.
 * @param val The string value, which is copied.
 */
void ht_strstr_insert(ht_strstr_t *ht, const char *key, const char *val);

/**
 * @brief Remove a string->string entry.
 * @param ht The table.
 * @param key The string key.
 */
void ht_strstr_remove(ht_strstr_t *ht, const char *key);

/**
 * @brief Insert a uint64 key mapping to a caller-owned pointer.
 * @param ht The table.
 * @param key The integer key.
 * @param val The pointer to store; the pointed-to object is not copied.
 */
void ht_u64ptr_insert(ht_u64ptr_t *ht, uint64_t key, void *val);

/**
 * @brief Remove a uint64->pointer entry. The pointed-to object is not freed.
 * @param ht The table.
 * @param key The integer key.
 */
void ht_u64ptr_remove(ht_u64ptr_t *ht, uint64_t key);

/**
 * @brief Insert a uint64 key mapping to a binary value.
 * @param ht The table.
 * @param key The integer key.
 * @param data Value bytes (may contain embedded NULs); may be NULL when size 0.
 * @param size Number of bytes at data.
 */
void ht_u64blob_insert(ht_u64blob_t *ht, uint64_t key, const void *data,
                       size_t size);

/**
 * @brief Remove a uint64->blob entry.
 * @param ht The table.
 * @param key The integer key.
 */
void ht_u64blob_remove(ht_u64blob_t *ht, uint64_t key);

/* Getting. */

/**
 * @brief Return the value stored under a key.
 * @param ht The table.
 * @param key The key.
 * @return The value, or NULL if absent.
 */
void *ht_get(const ht_t *ht, const void *key);

/**
 * @brief Look up the blob stored under a string key.
 * @param ht The table.
 * @param key NUL-terminated string key.
 * @param out_size If non-NULL, receives the blob length, or 0 if absent.
 * @return Pointer to the stored bytes (aliases table storage; do not free), or
 *         NULL if the key is absent.
 */
const void *ht_strblob_get(ht_strblob_t *ht, const char *key, size_t *out_size);

/**
 * @brief Return the double value stored under a string key.
 * @param ht The table.
 * @param key The string key.
 * @return Pointer to the stored double, or NULL if absent.
 */
void *ht_strdouble_get(ht_strdouble_t *ht, const char *key);

/**
 * @brief Return the float value stored under a string key.
 * @param ht The table.
 * @param key The string key.
 * @return Pointer to the stored float, or NULL if absent.
 */
void *ht_strfloat_get(ht_strfloat_t *ht, const char *key);

/**
 * @brief Return the int value stored under a string key.
 * @param ht The table.
 * @param key The string key.
 * @return Pointer to the stored int, or NULL if absent.
 */
void *ht_strint_get(ht_strint_t *ht, const char *key);

/**
 * @brief Return the pointer stored under a string key.
 * @param ht The table.
 * @param key The string key.
 * @return The stored pointer, or NULL if absent.
 */
void *ht_strptr_get(ht_strptr_t *ht, const char *key);

/**
 * @brief Return whether a member is present in a string set.
 * @param ht The set.
 * @param key The member.
 * @return true if present.
 */
bool ht_strset_contains(ht_strset_t *ht, const char *key);

/**
 * @brief Return the string value stored under a string key.
 * @param ht The table.
 * @param key The string key.
 * @return The stored string, or NULL if absent.
 */
const char *ht_strstr_get(ht_strstr_t *ht, const char *key);

/**
 * @brief Return the pointer stored under a uint64 key.
 * @param ht The table.
 * @param key The integer key.
 * @return The stored pointer, or NULL if absent.
 */
void *ht_u64ptr_get(ht_u64ptr_t *ht, uint64_t key);

/**
 * @brief Look up the blob stored under a uint64 key.
 * @param ht The table.
 * @param key The integer key.
 * @param out_size If non-NULL, receives the blob length, or 0 if absent.
 * @return Pointer to the stored bytes (aliases table storage; do not free), or
 *         NULL if the key is absent.
 */
const void *ht_u64blob_get(ht_u64blob_t *ht, uint64_t key, size_t *out_size);

/* Size, membership, and bulk operations. */

/**
 * @brief Return the number of entries.
 * @param ht The table.
 * @return The entry count (O(1)).
 */
size_t ht_size(const ht_t *ht);

/**
 * @brief Return whether a key is present.
 *
 * Unlike comparing ht_get against NULL, this distinguishes an absent key from a
 * key stored with a NULL value.
 *
 * @param ht The table.
 * @param key The key.
 * @return true if present.
 */
bool ht_contains(const ht_t *ht, const void *key);

/**
 * @brief Return the value under a key, inserting the pair first if absent.
 *
 * A single call avoids the time-of-check-to-time-of-use gap of a separate get
 * followed by insert.
 *
 * @param ht The table.
 * @param key The key.
 * @param val The value to insert if the key is absent.
 * @return The value now stored under the key.
 */
void *ht_get_or_insert(ht_t *ht, const void *key, const void *val);

/**
 * @brief Insert or replace a key/value pair, reporting which occurred.
 * @param ht The table.
 * @param key The key.
 * @param val The value.
 * @return true if the key was newly added, false if an existing value was
 *         replaced.
 */
bool ht_upsert(ht_t *ht, const void *key, const void *val);

/**
 * @brief Remove all entries, keeping the table allocated and reusable.
 * @param ht The table.
 */
void ht_clear(ht_t *ht);

/**
 * @brief Invoke a visitor for every entry.
 * @param ht The table.
 * @param visit Callback invoked as visit(key, val, user_data) per entry.
 * @param user_data Opaque pointer forwarded to the visitor.
 * @note The table must not be modified during iteration.
 */
void ht_foreach(const ht_t *ht, ht_visit visit, void *user_data);

/* Enumeration. An enumeration object is a cursor over a table's entries. */

/**
 * @brief Create a cursor over a generic table.
 * @param ht The table.
 * @return The cursor, or NULL on failure.
 */
ht_enum_t *ht_enum_create(ht_t *ht);

/**
 * @brief Advance a generic cursor.
 * @param he The cursor.
 * @param key Out-pointer for the key (may be NULL).
 * @param val Out-pointer for the value (may be NULL).
 * @return true if an entry was returned, false at the end.
 */
bool ht_enum_next(ht_enum_t *he, const void **key, const void **val);

/**
 * @brief Destroy a generic cursor.
 * @param he The cursor.
 */
void ht_enum_destroy(ht_enum_t *he);

/**
 * @brief Create a cursor over a string->blob table.
 * @param ht The table.
 * @return The cursor, or NULL on failure.
 */
ht_enum_t *ht_strblob_enum_create(ht_strblob_t *ht);

/**
 * @brief Advance a string->blob cursor.
 * @param he The cursor.
 * @param key Out-pointer for the key (may be NULL).
 * @param data Out-pointer for the blob bytes (may be NULL).
 * @param size Out-pointer for the blob length (may be NULL).
 * @return true if an entry was returned, false at the end.
 */
bool ht_strblob_enum_next(ht_enum_t *he, const char **key, const void **data,
                          size_t *size);

/**
 * @brief Destroy a string->blob cursor.
 * @param he The cursor.
 */
void ht_strblob_enum_destroy(ht_enum_t *he);

/**
 * @brief Create a cursor over a string->double table.
 * @param ht The table.
 * @return The cursor, or NULL on failure.
 */
ht_enum_t *ht_strdouble_enum_create(ht_strdouble_t *ht);

/**
 * @brief Advance a string->double cursor.
 * @param he The cursor.
 * @param key Out-pointer for the key (may be NULL).
 * @param val Out-pointer for the value (may be NULL).
 * @return true if an entry was returned, false at the end.
 */
bool ht_strdouble_enum_next(ht_enum_t *he, const char **key,
                            const double **val);

/**
 * @brief Destroy a string->double cursor.
 * @param he The cursor.
 */
void ht_strdouble_enum_destroy(ht_enum_t *he);

/**
 * @brief Create a cursor over a string->float table.
 * @param ht The table.
 * @return The cursor, or NULL on failure.
 */
ht_enum_t *ht_strfloat_enum_create(ht_strfloat_t *ht);

/**
 * @brief Advance a string->float cursor.
 * @param he The cursor.
 * @param key Out-pointer for the key (may be NULL).
 * @param val Out-pointer for the value (may be NULL).
 * @return true if an entry was returned, false at the end.
 */
bool ht_strfloat_enum_next(ht_enum_t *he, const char **key, const float **val);

/**
 * @brief Destroy a string->float cursor.
 * @param he The cursor.
 */
void ht_strfloat_enum_destroy(ht_enum_t *he);

/**
 * @brief Create a cursor over a string->int table.
 * @param ht The table.
 * @return The cursor, or NULL on failure.
 */
ht_enum_t *ht_strint_enum_create(ht_strint_t *ht);

/**
 * @brief Advance a string->int cursor.
 * @param he The cursor.
 * @param key Out-pointer for the key (may be NULL).
 * @param val Out-pointer for the value (may be NULL).
 * @return true if an entry was returned, false at the end.
 */
bool ht_strint_enum_next(ht_enum_t *he, const char **key, const int **val);

/**
 * @brief Destroy a string->int cursor.
 * @param he The cursor.
 */
void ht_strint_enum_destroy(ht_enum_t *he);

/**
 * @brief Create a cursor over a string->pointer table.
 * @param ht The table.
 * @return The cursor, or NULL on failure.
 */
ht_enum_t *ht_strptr_enum_create(ht_strptr_t *ht);

/**
 * @brief Advance a string->pointer cursor.
 * @param he The cursor.
 * @param key Out-pointer for the key (may be NULL).
 * @param val Out-pointer for the value (may be NULL).
 * @return true if an entry was returned, false at the end.
 */
bool ht_strptr_enum_next(ht_enum_t *he, const char **key, void **val);

/**
 * @brief Destroy a string->pointer cursor.
 * @param he The cursor.
 */
void ht_strptr_enum_destroy(ht_enum_t *he);

/**
 * @brief Create a cursor over a string set.
 * @param ht The set.
 * @return The cursor, or NULL on failure.
 */
ht_enum_t *ht_strset_enum_create(ht_strset_t *ht);

/**
 * @brief Advance a string-set cursor.
 * @param he The cursor.
 * @param key Out-pointer for the next member (may be NULL).
 * @return true if a member was returned, false at the end.
 */
bool ht_strset_enum_next(ht_enum_t *he, const char **key);

/**
 * @brief Destroy a string-set cursor.
 * @param he The cursor.
 */
void ht_strset_enum_destroy(ht_enum_t *he);

/**
 * @brief Create a cursor over a string->string table.
 * @param ht The table.
 * @return The cursor, or NULL on failure.
 */
ht_enum_t *ht_strstr_enum_create(ht_strstr_t *ht);

/**
 * @brief Advance a string->string cursor.
 * @param he The cursor.
 * @param key Out-pointer for the key (may be NULL).
 * @param val Out-pointer for the value (may be NULL).
 * @return true if an entry was returned, false at the end.
 */
bool ht_strstr_enum_next(ht_enum_t *he, const char **key, const char **val);

/**
 * @brief Destroy a string->string cursor.
 * @param he The cursor.
 */
void ht_strstr_enum_destroy(ht_enum_t *he);

/**
 * @brief Create a cursor over a uint64->pointer table.
 * @param ht The table.
 * @return The cursor, or NULL on failure.
 */
ht_enum_t *ht_u64ptr_enum_create(ht_u64ptr_t *ht);

/**
 * @brief Advance a uint64->pointer cursor.
 * @param he The cursor.
 * @param key Out-pointer for the key (may be NULL).
 * @param val Out-pointer for the value (may be NULL).
 * @return true if an entry was returned, false at the end.
 */
bool ht_u64ptr_enum_next(ht_enum_t *he, uint64_t *key, void **val);

/**
 * @brief Destroy a uint64->pointer cursor.
 * @param he The cursor.
 */
void ht_u64ptr_enum_destroy(ht_enum_t *he);

/**
 * @brief Create a cursor over a uint64->blob table.
 * @param ht The table.
 * @return The cursor, or NULL on failure.
 */
ht_enum_t *ht_u64blob_enum_create(ht_u64blob_t *ht);

/**
 * @brief Advance a uint64->blob cursor.
 * @param he The cursor.
 * @param key Out-pointer for the key (may be NULL).
 * @param data Out-pointer for the blob bytes (may be NULL).
 * @param size Out-pointer for the blob length (may be NULL).
 * @return true if an entry was returned, false at the end.
 */
bool ht_u64blob_enum_next(ht_enum_t *he, uint64_t *key, const void **data,
                          size_t *size);

/**
 * @brief Destroy a uint64->blob cursor.
 * @param he The cursor.
 */
void ht_u64blob_enum_destroy(ht_enum_t *he);

#ifdef __cplusplus
}
#endif

#endif // __HT_H__

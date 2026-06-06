/**
 * @file ht_random.c
 * @brief Cryptographically strong random bytes.
 *
 * Project: libhashtable
 * URL: https://github.com/berrym/libhashtable
 *
 * @author Michael Berry <trismegustis@gmail.com>
 * @copyright Copyright (c) 2024 Michael Berry
 * @license MIT
 */

#include "ht.h"

/* All platform-specific entropy source selection is isolated to this
 * translation unit so the public API stays platform-neutral. */
#if defined(_WIN32)
#include <windows.h>
/* bcrypt.h must follow windows.h. */
#include <bcrypt.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) ||    \
    defined(__NetBSD__) || defined(__DragonFly__)
#include <stdlib.h>
#define HT_RANDOM_ARC4RANDOM 1
#else
#if defined(__linux__)
#include <sys/random.h>
#define HT_RANDOM_GETRANDOM 1
#endif
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

/// Fill buf with len cryptographically strong random bytes. Returns 0 on
/// success and -1 on failure. On failure the bytes of buf are unspecified and
/// must not be used; a weak entropy source is never substituted.
int ht_random_bytes(void *buf, size_t len) {
    if (!buf) {
        return -1;
    }
    if (len == 0) {
        return 0;
    }

#if defined(_WIN32)
    NTSTATUS status = BCryptGenRandom(NULL, (PUCHAR)buf, (ULONG)len,
                                      BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return BCRYPT_SUCCESS(status) ? 0 : -1;

#elif defined(HT_RANDOM_ARC4RANDOM)
    arc4random_buf(buf, len);
    return 0;

#else
    unsigned char *p = (unsigned char *)buf;
    size_t filled = 0;

#if defined(HT_RANDOM_GETRANDOM)
    while (filled < len) {
        ssize_t n = getrandom(p + filled, len - filled, 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == ENOSYS) {
                break; /* syscall unavailable: fall back to /dev/urandom */
            }
            return -1;
        }
        filled += (size_t)n;
    }
    if (filled == len) {
        return 0;
    }
#endif

    /* /dev/urandom fallback. This device is a Unix convention, not specified
     * by POSIX. /dev/random is never used. */
    int fd;
    do {
        fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0) {
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0 || !S_ISCHR(st.st_mode)) {
        close(fd);
        return -1;
    }

    while (filled < len) {
        ssize_t n = read(fd, p + filled, len - filled);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            close(fd);
            return -1;
        }
        if (n == 0) {
            close(fd); /* unexpected end of file */
            return -1;
        }
        filled += (size_t)n;
    }

    close(fd);
    return 0;
#endif
}

#if HAVE_MMAP
/* for posix_fallocate */
#define _XOPEN_SOURCE 600
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

#include "upro/common.h"
#include "upro/error.h"
#include "upro/mmap.h"
#include "upro/ecurve.h"

struct mmap_header {
    char alphabet_str[UPRO_ALPHABET_SIZE];
    size_t suffix_count;
};

#define SIZE_HEADER (sizeof (struct mmap_header))
#define SIZE_PREFIXES ((UPRO_PREFIX_MAX + 1) * sizeof (struct upro_ecurve_pfxtable))
#define SIZE_SUFFIXES(suffix_count) ((suffix_count) * sizeof (upro_suffix))
#define SIZE_CLASSES(suffix_count) ((suffix_count) * sizeof (upro_family))
#define SIZE_TOTAL(suffix_count) \
    (SIZE_HEADER + \
     SIZE_PREFIXES + \
     SIZE_SUFFIXES(suffix_count) + \
     SIZE_CLASSES(suffix_count))

#define OFFSET_PREFIXES (SIZE_HEADER)
#define OFFSET_SUFFIXES (OFFSET_PREFIXES + SIZE_PREFIXES)
#define OFFSET_CLASSES(suffix_count) (OFFSET_SUFFIXES + SIZE_SUFFIXES(suffix_count))

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif
#ifndef MAP_POPULATE
#define MAP_NORESERVE 0
#endif

static int
mmap_map(struct upro_ecurve *ecurve, const char *path)
{
#if HAVE_MMAP
    int res;
    struct stat st;
    struct mmap_header *header;
    char *region, alphabet_str[UPRO_ALPHABET_SIZE + 1];

    ecurve->mmap_fd = open(path, O_RDONLY);
    if (ecurve->mmap_fd == -1) {
        return upro_error(UPRO_ERRNO);
    }

    if (fstat(ecurve->mmap_fd, &st) == -1) {
        return upro_error(UPRO_ERRNO);
    }
    ecurve->mmap_size = st.st_size;
    ecurve->mmap_ptr =
        region = mmap(NULL, ecurve->mmap_size, PROT_READ,
                MAP_PRIVATE | MAP_NORESERVE | MAP_POPULATE,
                ecurve->mmap_fd, 0);

    if (ecurve->mmap_ptr == MAP_FAILED) {
        res = upro_error(UPRO_ERRNO);
        goto error_close;
    }

    posix_madvise(ecurve->mmap_ptr, ecurve->mmap_size, POSIX_MADV_WILLNEED);
    posix_madvise(ecurve->mmap_ptr, ecurve->mmap_size, POSIX_MADV_RANDOM);

    header = ecurve->mmap_ptr;
    if (ecurve->mmap_size != SIZE_TOTAL(header->suffix_count)) {
        res = upro_error(UPRO_ERRNO);
        goto error_munmap;
    }

    memcpy(alphabet_str, header->alphabet_str, UPRO_ALPHABET_SIZE);
    alphabet_str[UPRO_ALPHABET_SIZE] = '\0';
    res = upro_alphabet_init(&ecurve->alphabet, alphabet_str);
    if (res) {
        goto error_munmap;
    }

    ecurve->suffix_count = header->suffix_count;
    ecurve->prefixes = (void *)(region + OFFSET_PREFIXES);
    ecurve->suffixes = (void *)(region + OFFSET_SUFFIXES);
    ecurve->families =  (void *)(region + OFFSET_CLASSES(ecurve->suffix_count));

    return 0;

error_munmap:
    munmap(ecurve->mmap_ptr, ecurve->mmap_size);
error_close:
    close(ecurve->mmap_fd);
    return res;
#else
    (void) ecurve;
    (void) path;
    return upro_error(UPRO_ENOTSUP);;
#endif
}

int
upro_mmap_map(struct upro_ecurve *ecurve, const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = upro_mmap_mapv(ecurve, pathfmt, ap);
    va_end(ap);
    return res;
}

int
upro_mmap_mapv(struct upro_ecurve *ecurve, const char *pathfmt, va_list ap)
{
    int res;
    char *buf;
    size_t n;
    va_list aq;

    va_copy(aq, ap);
    n = vsnprintf(NULL, 0, pathfmt, aq);
    va_end(aq);

    buf = malloc(n + 1);
    if (!buf) {
        return upro_error(UPRO_ENOMEM);
    }
    vsprintf(buf, pathfmt, ap);

    res = mmap_map(ecurve, buf);
    free(buf);
    return res;
}

void
upro_mmap_unmap(struct upro_ecurve *ecurve)
{
#if HAVE_MMAP
    munmap(ecurve->mmap_ptr, ecurve->mmap_size);
    close(ecurve->mmap_fd);
#else
    (void) ecurve;
#endif
}

static int
mmap_store(const struct upro_ecurve *ecurve, const char *path)
{
#if HAVE_MMAP
    int fd, res = 0;
    size_t size;
    char *region;
    struct mmap_header header;

    size = SIZE_TOTAL(ecurve->suffix_count);

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        return upro_error(UPRO_ERRNO);
    }

    if (posix_fallocate(fd, 0, size)) {
        res = upro_error(UPRO_ERRNO);
        goto error_close;
    }

    region = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (region == MAP_FAILED) {
        res = upro_error(UPRO_ERRNO);
        goto error_close;
    }

    header.suffix_count = ecurve->suffix_count;
    memcpy(&header.alphabet_str, ecurve->alphabet.str, UPRO_ALPHABET_SIZE);

    memcpy(region, &header, SIZE_HEADER);
    memcpy(region + OFFSET_PREFIXES, ecurve->prefixes, SIZE_PREFIXES);
    memcpy(region + OFFSET_SUFFIXES,
           ecurve->suffixes, SIZE_SUFFIXES(ecurve->suffix_count));

    memcpy(region + OFFSET_CLASSES(ecurve->suffix_count),
           ecurve->families, SIZE_CLASSES(ecurve->suffix_count));

    munmap(region, size);
    close(fd);
    return 0;

error_close:
    close(fd);
    return res;
#else
    (void) ecurve;
    (void) path;
    return upro_error(UPRO_ENOTSUP);
#endif
}
int
upro_mmap_store(const struct upro_ecurve *ecurve, const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = upro_mmap_storev(ecurve, pathfmt, ap);
    va_end(ap);
    return res;
}

int
upro_mmap_storev(const struct upro_ecurve *ecurve, const char *pathfmt,
        va_list ap)
{
    int res;
    char *buf;
    size_t n;
    va_list aq;

    va_copy(aq, ap);
    n = vsnprintf(NULL, 0, pathfmt, aq);
    va_end(aq);

    buf = malloc(n + 1);
    if (!buf) {
        return upro_error(UPRO_ENOMEM);
    }
    vsprintf(buf, pathfmt, ap);

    res = mmap_store(ecurve, buf);
    free(buf);
    return res;
}

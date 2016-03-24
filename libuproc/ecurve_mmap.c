/* Map a file to an ecurve
 *
 * Copyright 2014 Peter Meinicke, Robin Martinjak
 *
 * This file is part of libuproc.
 *
 * libuproc is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libuproc is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libuproc.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if HAVE_MMAP && USE_MMAP
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/ecurve.h"

#include "ecurve_internal.h"

struct mmap_header_v1 {
    char alphabet_str[UPROC_ALPHABET_SIZE];
    size_t suffix_count;
};

struct mmap_header {
    char alphabet_str[UPROC_ALPHABET_SIZE];
    uproc_rank ranks_count;
    size_t suffix_count;
};

static const uint64_t magic_number = 0xd2eadfUL;

#define SIZE_HEADER (sizeof(struct mmap_header))
#define SIZE_PREFIXES \
    ((UPROC_PREFIX_MAX + 1) * sizeof(struct uproc_ecurve_pfxtable))
#define SIZE_SUFFIXES(suffix_count) ((suffix_count) * sizeof(uproc_suffix))
#define SIZE_CLASSES(ranks_count, suffix_count) \
    ((suffix_count) * (ranks_count) * sizeof(uproc_class))
#define SIZE_TOTAL(ranks_count, suffix_count)                    \
    (SIZE_HEADER + SIZE_PREFIXES + SIZE_SUFFIXES(suffix_count) + \
     SIZE_CLASSES((ranks_count), (suffix_count)) + (3 * sizeof magic_number))

#define OFFSET_PREFIXES (SIZE_HEADER)
#define OFFSET_MAGIC1 (OFFSET_PREFIXES + SIZE_PREFIXES)
#define OFFSET_SUFFIXES (OFFSET_MAGIC1 + (sizeof magic_number))
#define OFFSET_MAGIC2(suffix_count) \
    (OFFSET_SUFFIXES + SIZE_SUFFIXES(suffix_count))
#define OFFSET_CLASSES(suffix_count) \
    (OFFSET_MAGIC2(suffix_count) + (sizeof magic_number))
#define OFFSET_MAGIC3(ranks_count, suffix_count) \
    (OFFSET_CLASSES(suffix_count) + SIZE_CLASSES((ranks_count), (suffix_count)))

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif
#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif

static void fill_header_v1(struct mmap_header *header, void *region,
                           unsigned char **after_header)
{
    struct mmap_header_v1 *v1_header = region;
    memcpy(header->alphabet_str, v1_header->alphabet_str,
           sizeof header->alphabet_str);
    header->ranks_count = 1;
    header->suffix_count = v1_header->suffix_count;
    *after_header = region + sizeof *v1_header;
}

static void fill_header_v2(struct mmap_header *header, void *region,
                           unsigned char **after_header)
{
    struct mmap_header *v2_header = region;
    memcpy(header->alphabet_str, v2_header->alphabet_str,
           sizeof header->alphabet_str);
    header->ranks_count = v2_header->ranks_count;
    header->suffix_count = v2_header->suffix_count;
    *after_header = region + sizeof *v2_header;
}

static uproc_ecurve *ecurve_map(const char *path, int version)
{
#if HAVE_MMAP && USE_MMAP
    struct stat st;
    char alphabet_str[UPROC_ALPHABET_SIZE + 1];
    struct uproc_ecurve_s *ec = malloc(sizeof *ec);

    if (!ec) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    *ec = (struct uproc_ecurve_s){0};

    ec->mmap_fd = open(path, O_RDWR);
    if (ec->mmap_fd == -1) {
        uproc_error_msg(UPROC_ERRNO, "failed to open %s", path);
        goto error;
    }

    if (fstat(ec->mmap_fd, &st) == -1) {
        uproc_error_msg(UPROC_ERRNO, "stat failed");
        goto error_close;
    }
    ec->mmap_size = st.st_size;
    ec->mmap_ptr =
        mmap(NULL, ec->mmap_size, PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_NORESERVE | MAP_POPULATE, ec->mmap_fd, 0);

    if (ec->mmap_ptr == MAP_FAILED) {
        uproc_error_msg(UPROC_ERRNO, "mmap failed");
        goto error_close;
    }

#if HAVE_POSIX_MADVISE
#ifdef POSIX_MADV_WILLNEED
    posix_madvise(ec->mmap_ptr, ec->mmap_size, POSIX_MADV_WILLNEED);
#endif
#ifdef POSIX_MADV_RANDOM
    posix_madvise(ec->mmap_ptr, ec->mmap_size, POSIX_MADV_RANDOM);
#endif
#endif

    unsigned char *region = NULL;
    struct mmap_header header;
    if (version == 1) {
        fill_header_v1(&header, ec->mmap_ptr, &region);
    } else {
        fill_header_v2(&header, ec->mmap_ptr, &region);
    }

#if 0
    if (ec->mmap_size !=
        SIZE_TOTAL(header.ranks_count, header.suffix_count)) {
        uproc_error(UPROC_EINVAL);
        goto error_munmap;
    }
#endif

    ec->ranks_count = header.ranks_count;
    ec->suffix_count = header.suffix_count;

    memcpy(alphabet_str, header.alphabet_str, UPROC_ALPHABET_SIZE);
    alphabet_str[UPROC_ALPHABET_SIZE] = '\0';
    ec->alphabet = uproc_alphabet_create(alphabet_str);
    if (!ec->alphabet) {
        goto error_munmap;
    }

    ec->prefixes = (void *)region;
    region += SIZE_PREFIXES;

    uint64_t *magic;
    magic = (void *)region;
    if (*magic != magic_number) {
        uproc_error_msg(UPROC_EINVAL, "inconsistent magic number 1");
        goto error_munmap;
    }
    region += sizeof magic_number;

    ec->suffixes = (void *)region;
    region += SIZE_SUFFIXES(ec->suffix_count);

    magic = (void *)region;
    if (*magic != magic_number) {
        uproc_error_msg(UPROC_EINVAL, "inconsistent magic number 2");
        goto error_munmap;
    }
    region += sizeof magic_number;

    ec->classes = (void *)region;
    region += SIZE_CLASSES(ec->ranks_count, ec->suffix_count);

    magic = (void *)region;
    if (*magic != magic_number) {
        uproc_error_msg(UPROC_EINVAL, "inconsistent magic number 3");
        goto error_munmap;
    }
    region += sizeof magic_number;

    uproc_assert(ec->mmap_ptr + ec->mmap_size == region);

    return ec;

error_munmap:
    munmap(ec->mmap_ptr, ec->mmap_size);
error_close:
    close(ec->mmap_fd);
error:
    free(ec);
    return NULL;
#else
    (void)path;
    uproc_error(UPROC_ENOTSUP);
    return NULL;
#endif
}

uproc_ecurve *uproc_ecurve_mmap(const char *pathfmt, ...)
{
    struct uproc_ecurve_s *ec;
    va_list ap;
    va_start(ap, pathfmt);
    ec = uproc_ecurve_mmapv(pathfmt, ap);
    va_end(ap);
    return ec;
}

static uproc_ecurve *ecurve_mmapv(int version, const char *pathfmt, va_list ap)
{
    struct uproc_ecurve_s *ec;
    char *buf;
    size_t n;
    va_list aq;

    va_copy(aq, ap);
    n = vsnprintf(NULL, 0, pathfmt, aq);
    va_end(aq);

    buf = malloc(n + 1);
    if (!buf) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    vsprintf(buf, pathfmt, ap);

    ec = ecurve_map(buf, version);
    free(buf);
    return ec;
}

uproc_ecurve *uproc_ecurve_mmapv_v1(const char *pathfmt, va_list ap)
{
    return ecurve_mmapv(1, pathfmt, ap);
}

uproc_ecurve *uproc_ecurve_mmapv(const char *pathfmt, va_list ap)
{
    return ecurve_mmapv(2, pathfmt, ap);
}

void uproc_ecurve_munmap(struct uproc_ecurve_s *ecurve)
{
#if HAVE_MMAP && USE_MMAP
    munmap(ecurve->mmap_ptr, ecurve->mmap_size);
    close(ecurve->mmap_fd);
#else
    (void)ecurve;
#endif
}

static int mmap_store(const struct uproc_ecurve_s *ecurve, const char *path)
{
#if HAVE_MMAP && USE_MMAP
    int fd, res = 0;
    size_t size;
    char *region;

    size = SIZE_TOTAL(ecurve->ranks_count, ecurve->suffix_count);

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        return uproc_error_msg(UPROC_ERRNO, "failed to open %s", path);
    }

    if (ftruncate(fd, size)) {
        res = uproc_error_msg(UPROC_ERRNO, "failed to allocate space");
        goto error_close;
    }

    region = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (region == MAP_FAILED) {
        res = uproc_error_msg(UPROC_ERRNO, "mmap failed");
        goto error_close;
    }

    struct mmap_header header = {.ranks_count = ecurve->ranks_count,
                                 .suffix_count = ecurve->suffix_count};
    memcpy(&header.alphabet_str, uproc_alphabet_str(ecurve->alphabet),
           UPROC_ALPHABET_SIZE);

    memcpy(region, &header, SIZE_HEADER);
    memcpy(region + OFFSET_PREFIXES, ecurve->prefixes, SIZE_PREFIXES);
    memcpy(region + OFFSET_MAGIC1, &magic_number, sizeof magic_number);
    memcpy(region + OFFSET_SUFFIXES, ecurve->suffixes,
           SIZE_SUFFIXES(ecurve->suffix_count));
    memcpy(region + OFFSET_MAGIC2(ecurve->suffix_count), &magic_number,
           sizeof magic_number);
    memcpy(region + OFFSET_CLASSES(ecurve->suffix_count), ecurve->classes,
           SIZE_CLASSES(ecurve->ranks_count, ecurve->suffix_count));
    memcpy(region + OFFSET_MAGIC3(ecurve->ranks_count, ecurve->suffix_count),
           &magic_number, sizeof magic_number);

    munmap(region, size);
    close(fd);
    return 0;

error_close:
    close(fd);
    return res;
#else
    (void)ecurve;
    (void)path;
    return uproc_error(UPROC_ENOTSUP);
#endif
}

int uproc_ecurve_mmap_store(const uproc_ecurve *ecurve, const char *pathfmt,
                            ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_ecurve_mmap_storev(ecurve, pathfmt, ap);
    va_end(ap);
    return res;
}

int uproc_ecurve_mmap_storev(const uproc_ecurve *ecurve, const char *pathfmt,
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
        return uproc_error(UPROC_ENOMEM);
    }
    vsprintf(buf, pathfmt, ap);

    res = mmap_store(ecurve, buf);
    free(buf);
    return res;
}

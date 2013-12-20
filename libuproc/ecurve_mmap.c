/* Map a file to an ecurve
 *
 * Copyright 2013 Peter Meinicke, Robin Martinjak
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

#if HAVE_MMAP
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

struct mmap_header
{
    char alphabet_str[UPROC_ALPHABET_SIZE];
    size_t suffix_count;
};

static const uint64_t magic_number = 0xd2eadfUL;

#define SIZE_HEADER (sizeof (struct mmap_header))
#define SIZE_PREFIXES \
    ((UPROC_PREFIX_MAX + 1) * \
     sizeof (*((struct uproc_ecurve_s*)0)->prefixes))
#define SIZE_SUFFIXES(suffix_count) \
    ((suffix_count) * \
     sizeof (*((struct uproc_ecurve_s*)0)->suffixes))
#define SIZE_TOTAL(suffix_count) \
    (SIZE_HEADER + SIZE_PREFIXES + SIZE_SUFFIXES(suffix_count) + \
     (2 * sizeof magic_number))

#define OFFSET_PREFIXES (SIZE_HEADER)
#define OFFSET_MAGIC1 (OFFSET_PREFIXES + SIZE_PREFIXES)
#define OFFSET_SUFFIXES (OFFSET_MAGIC1 + (sizeof magic_number))
#define OFFSET_MAGIC2(suffix_count) (OFFSET_SUFFIXES + SIZE_SUFFIXES(suffix_count))

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif
#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif

static uproc_ecurve *
ecurve_map(const char *path)
{
#if HAVE_MMAP
    struct stat st;
    struct mmap_header *header;
    char alphabet_str[UPROC_ALPHABET_SIZE + 1];
    struct uproc_ecurve_s *ec = malloc(sizeof *ec);

    if (!ec) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }

    ec->mmap_fd = open(path, O_RDONLY);
    if (ec->mmap_fd == -1) {
        uproc_error(UPROC_ERRNO);
        goto error;
    }

    if (fstat(ec->mmap_fd, &st) == -1) {
        uproc_error(UPROC_ERRNO);
        goto error_close;
    }
    ec->mmap_size = st.st_size;
    ec->mmap_ptr = mmap(NULL, ec->mmap_size, PROT_READ,
                        MAP_PRIVATE | MAP_NORESERVE | MAP_POPULATE,
                        ec->mmap_fd, 0);

    if (ec->mmap_ptr == MAP_FAILED) {
        uproc_error(UPROC_ERRNO);
        goto error_close;
    }

#if HAVE_POSIX_MADVISE
    posix_madvise(ec->mmap_ptr, ec->mmap_size, POSIX_MADV_WILLNEED);
    posix_madvise(ec->mmap_ptr, ec->mmap_size, POSIX_MADV_RANDOM);
#endif

    header = ec->mmap_ptr;
    if (ec->mmap_size != SIZE_TOTAL(header->suffix_count)) {
        uproc_error(UPROC_ERRNO);
        goto error_munmap;
    }
    ec->suffix_count = header->suffix_count;

    memcpy(alphabet_str, header->alphabet_str, UPROC_ALPHABET_SIZE);
    alphabet_str[UPROC_ALPHABET_SIZE] = '\0';
    ec->alphabet = uproc_alphabet_create(alphabet_str);
    if (!ec->alphabet) {
        goto error_munmap;
    }

   ec->prefixes = (void *)(ec->mmap_ptr + OFFSET_PREFIXES);
   ec->suffixes = (void *)(ec->mmap_ptr + OFFSET_SUFFIXES);
   uint64_t *m1, *m2, *m3;
   m1 = (void*)(ec->mmap_ptr + OFFSET_MAGIC1);
   m2 = (void*)(ec->mmap_ptr + OFFSET_MAGIC2(ec->suffix_count));
   if (*m1 != magic_number || *m2 != magic_number) {
       uproc_error_msg(UPROC_EINVAL, "inconsistent magic number");
       goto error_munmap;
   }
   return ec;

error_munmap:
    munmap(ec->mmap_ptr, ec->mmap_size);
error_close:
    close(ec->mmap_fd);
error:
    return NULL;
#else
    (void) path;
    uproc_error(UPROC_ENOTSUP);
    return NULL;
#endif
}

uproc_ecurve *
uproc_ecurve_mmap(const char *pathfmt, ...)
{
    struct uproc_ecurve_s *ec;
    va_list ap;
    va_start(ap, pathfmt);
    ec = uproc_ecurve_mmapv(pathfmt, ap);
    va_end(ap);
    return ec;
}

uproc_ecurve *
uproc_ecurve_mmapv(const char *pathfmt, va_list ap)
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

    ec = ecurve_map(buf);
    free(buf);
    return ec;
}

void
uproc_ecurve_munmap(struct uproc_ecurve_s *ecurve)
{
#if HAVE_MMAP
    munmap(ecurve->mmap_ptr, ecurve->mmap_size);
    close(ecurve->mmap_fd);
#else
    (void) ecurve;
#endif
}

static int
mmap_store(const struct uproc_ecurve_s *ecurve, const char *path)
{
#if HAVE_MMAP
    int fd, res = 0;
    size_t size;
    char *region;
    struct mmap_header header;

    size = SIZE_TOTAL(ecurve->suffix_count);

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        return uproc_error(UPROC_ERRNO);
    }

    if (posix_fallocate(fd, 0, size)) {
        res = uproc_error(UPROC_ERRNO);
        goto error_close;
    }

    region = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (region == MAP_FAILED) {
        res = uproc_error(UPROC_ERRNO);
        goto error_close;
    }

    header.suffix_count = ecurve->suffix_count;
    memcpy(&header.alphabet_str, uproc_alphabet_str(ecurve->alphabet),
           UPROC_ALPHABET_SIZE);

    memcpy(region, &header, SIZE_HEADER);
    memcpy(region + OFFSET_PREFIXES, ecurve->prefixes, SIZE_PREFIXES);
    memcpy(region + OFFSET_MAGIC1, &magic_number, sizeof magic_number);
    memcpy(region + OFFSET_SUFFIXES, ecurve->suffixes,
           SIZE_SUFFIXES(ecurve->suffix_count));
    memcpy(region + OFFSET_MAGIC2(ecurve->suffix_count), &magic_number, sizeof magic_number);
    munmap(region, size);
    close(fd);
    return 0;

error_close:
    close(fd);
    return res;
#else
    (void) ecurve;
    (void) path;
    return uproc_error(UPROC_ENOTSUP);
#endif
}

int
uproc_ecurve_mmap_store(const uproc_ecurve *ecurve, const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_ecurve_mmap_storev(ecurve, pathfmt, ap);
    va_end(ap);
    return res;
}

int
uproc_ecurve_mmap_storev(const uproc_ecurve *ecurve, const char *pathfmt,
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

/* for posix_fallocate */
#define _XOPEN_SOURCE 600
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "ecurve/common.h"
#include "ecurve/mmap.h"
#include "ecurve/ecurve.h"

struct mmap_header {
    char alphabet_str[EC_ALPHABET_SIZE];
    size_t suffix_count;
};

#define SIZE_HEADER (sizeof (struct mmap_header))
#define SIZE_PFXTABLE ((EC_PREFIX_MAX + 1) * sizeof (struct ec_ecurve_pfxtable))
#define SIZE_SUFFIXES(suffix_count) ((suffix_count) * sizeof (ec_suffix))
#define SIZE_CLASSES(suffix_count) ((suffix_count) * sizeof (ec_class))
#define SIZE_TOTAL(suffix_count) \
    (SIZE_HEADER + \
     SIZE_PFXTABLE + \
     SIZE_SUFFIXES(suffix_count) + \
     SIZE_CLASSES(suffix_count))

#define OFFSET_PFXTABLE (SIZE_HEADER)
#define OFFSET_SUFFIXES (OFFSET_PFXTABLE + SIZE_PFXTABLE)
#define OFFSET_CLASSES(suffix_count) (OFFSET_SUFFIXES + SIZE_SUFFIXES(suffix_count))

int
ec_mmap_map(ec_ecurve *ecurve, const char *path)
{
    struct ec_ecurve_s *e = ecurve;
    struct stat st;
    struct mmap_header *header;
    char *region, alphabet_str[EC_ALPHABET_SIZE + 1];

    e->mmap_fd = open(path, O_RDONLY);
    if (e->mmap_fd == -1) {
        return EC_FAILURE;
    }

    if (fstat(e->mmap_fd, &st) == -1) {
        return EC_FAILURE;
    }
    e->mmap_size = st.st_size;
    e->mmap_ptr = region = mmap(NULL, e->mmap_size, PROT_READ, MAP_PRIVATE, e->mmap_fd, 0);

    if (e->mmap_ptr == MAP_FAILED) {
        goto error_close;
    }

    header = e->mmap_ptr;
    memcpy(alphabet_str, header->alphabet_str, EC_ALPHABET_SIZE);
    alphabet_str[EC_ALPHABET_SIZE] = '\0';
    if (ec_alphabet_init(&e->alphabet, alphabet_str) == EC_FAILURE) {
        goto error_munmap;
    }

    e->suffix_count = header->suffix_count;
    /* file is too small */
    if (e->mmap_size < SIZE_TOTAL(e->suffix_count)) {
        goto error_munmap;
    }

    e->prefix_table = (void *)(region + OFFSET_PFXTABLE);
    e->suffix_table = (void *)(region + OFFSET_SUFFIXES);
    e->class_table =  (void *)(region + OFFSET_CLASSES(e->suffix_count));

    return EC_SUCCESS;

error_munmap:
    munmap(e->mmap_ptr, e->mmap_size);
error_close:
    close(e->mmap_fd);
    return EC_FAILURE;
}

void
ec_mmap_unmap(ec_ecurve *ecurve)
{
    struct ec_ecurve_s *e = ecurve;
    munmap(e->mmap_ptr, e->mmap_size);
    close(e->mmap_fd);
}

int
ec_mmap_store(const ec_ecurve *ecurve, const char *path)
{
    int fd;
    const struct ec_ecurve_s *e = ecurve;
    size_t size;
    char *region;
    struct mmap_header header;

    size = SIZE_TOTAL(e->suffix_count);

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        return EC_FAILURE;
    }

    if (posix_fallocate(fd, 0, size)) {
        return EC_FAILURE;
    }

    region = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (region == MAP_FAILED) {
        goto error_close;
    }

    header.suffix_count = e->suffix_count;
    memcpy(&header.alphabet_str, e->alphabet.str, EC_ALPHABET_SIZE);

    memcpy(region, &header, SIZE_HEADER);
    memcpy(region + OFFSET_PFXTABLE, e->prefix_table, SIZE_PFXTABLE);
    memcpy(region + OFFSET_SUFFIXES,
           e->suffix_table, SIZE_SUFFIXES(e->suffix_count));

    memcpy(region + OFFSET_CLASSES(e->suffix_count),
           e->class_table, SIZE_CLASSES(e->suffix_count));

    munmap(region, size);
    close(fd);
    return EC_SUCCESS;

error_close:
    close(fd);
    return EC_FAILURE;
}

#if HAVE_MMAP
/* for posix_fallocate */
#define _XOPEN_SOURCE 600
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

#include "ecurve/common.h"
#include "ecurve/mmap.h"
#include "ecurve/ecurve.h"

struct mmap_header {
    char alphabet_str[EC_ALPHABET_SIZE];
    size_t suffix_count;
};

#define SIZE_HEADER (sizeof (struct mmap_header))
#define SIZE_PREFIXES ((EC_PREFIX_MAX + 1) * sizeof (struct ec_ecurve_pfxtable))
#define SIZE_SUFFIXES(suffix_count) ((suffix_count) * sizeof (ec_suffix))
#define SIZE_CLASSES(suffix_count) ((suffix_count) * sizeof (ec_class))
#define SIZE_TOTAL(suffix_count) \
    (SIZE_HEADER + \
     SIZE_PREFIXES + \
     SIZE_SUFFIXES(suffix_count) + \
     SIZE_CLASSES(suffix_count))

#define OFFSET_PREFIXES (SIZE_HEADER)
#define OFFSET_SUFFIXES (OFFSET_PREFIXES + SIZE_PREFIXES)
#define OFFSET_CLASSES(suffix_count) (OFFSET_SUFFIXES + SIZE_SUFFIXES(suffix_count))

int
ec_mmap_map(struct ec_ecurve *ecurve, const char *path)
{
#if HAVE_MMAP
    int res;
    struct stat st;
    struct mmap_header *header;
    char *region, alphabet_str[EC_ALPHABET_SIZE + 1];

    ecurve->mmap_fd = open(path, O_RDONLY);
    if (ecurve->mmap_fd == -1) {
        return EC_ESYSCALL;
    }

    if (fstat(ecurve->mmap_fd, &st) == -1) {
        return EC_ESYSCALL;
    }
    ecurve->mmap_size = st.st_size;
    ecurve->mmap_ptr =
        region = mmap(NULL, ecurve->mmap_size, PROT_READ, MAP_PRIVATE,
                      ecurve->mmap_fd, 0);

    if (ecurve->mmap_ptr == MAP_FAILED) {
        res = EC_ESYSCALL;
        goto error_close;
    }

    header = ecurve->mmap_ptr;
    if (ecurve->mmap_size != SIZE_TOTAL(header->suffix_count)) {
        res = EC_EINVAL;
        goto error_munmap;
    }

    memcpy(alphabet_str, header->alphabet_str, EC_ALPHABET_SIZE);
    alphabet_str[EC_ALPHABET_SIZE] = '\0';
    res = ec_alphabet_init(&ecurve->alphabet, alphabet_str);
    if (EC_ISERROR(res)) {
        goto error_munmap;
    }

    ecurve->suffix_count = header->suffix_count;
    ecurve->prefixes = (void *)(region + OFFSET_PREFIXES);
    ecurve->suffixes = (void *)(region + OFFSET_SUFFIXES);
    ecurve->classes =  (void *)(region + OFFSET_CLASSES(ecurve->suffix_count));

    return EC_SUCCESS;

error_munmap:
    munmap(ecurve->mmap_ptr, ecurve->mmap_size);
error_close:
    close(ecurve->mmap_fd);
#else
    (void) ecurve;
    (void) path;
#endif
    return res;
}

void
ec_mmap_unmap(struct ec_ecurve *ecurve)
{
#if HAVE_MMAP
    munmap(ecurve->mmap_ptr, ecurve->mmap_size);
    close(ecurve->mmap_fd);
#else
    (void) ecurve;
#endif
}

int
ec_mmap_store(const struct ec_ecurve *ecurve, const char *path)
{
#if HAVE_MMAP
    int fd, res = EC_SUCCESS;
    size_t size;
    char *region;
    struct mmap_header header;

    size = SIZE_TOTAL(ecurve->suffix_count);

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        return EC_ESYSCALL;
    }

    if (posix_fallocate(fd, 0, size)) {
        res = EC_ESYSCALL;
        goto error_close;
    }

    region = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (region == MAP_FAILED) {
        res = EC_ESYSCALL;
        goto error_close;
    }

    header.suffix_count = ecurve->suffix_count;
    memcpy(&header.alphabet_str, ecurve->alphabet.str, EC_ALPHABET_SIZE);

    memcpy(region, &header, SIZE_HEADER);
    memcpy(region + OFFSET_PREFIXES, ecurve->prefixes, SIZE_PREFIXES);
    memcpy(region + OFFSET_SUFFIXES,
           ecurve->suffixes, SIZE_SUFFIXES(ecurve->suffix_count));

    memcpy(region + OFFSET_CLASSES(ecurve->suffix_count),
           ecurve->classes, SIZE_CLASSES(ecurve->suffix_count));

    munmap(region, size);
    close(fd);
    return EC_SUCCESS;

error_close:
    close(fd);
#else
    (void) ecurve;
    (void) path;
#endif
    return res;
}

#ifndef UPROC_ECURVE_INTERNAL_H
#define UPROC_ECURVE_INTERNAL_H

#include <stdint.h>

typedef uint_least32_t pfxtab_suffix;
#define PFXTAB_SUFFIX_MAX UINT_LEAST32_MAX

typedef uint_least16_t pfxtab_neigh;
#define PFXTAB_NEIGH_MAX UINT_LEAST16_MAX

typedef uint_least16_t pfxtab_count;
#define ECURVE_EDGE ((pfxtab_count) -1)
#define ECURVE_ISEDGE(p) ((p).count == ECURVE_EDGE)

/** Struct defining an ecurve */
struct uproc_ecurve_s
{
    /** Translation alphabet */
    uproc_alphabet *alphabet;

    /** Number of suffixes */
    size_t suffix_count;

    /** Table of suffixes
     *
     * Will be allocated to hold `#suffix_count` objects
     */
    uproc_suffix *suffixes;

    /** Table of families associated with the suffixes
     *
     * Will be allocated to hold `#suffix_count` objects
     */
    uproc_family *families;

    /** Table that maps prefixes to entries in the ecurve's suffix table */
    struct uproc_ecurve_pfxtable {
        union {
            /** Index of the first associated entry in #suffixes */
            pfxtab_suffix first;

            /* indicates offsets towards the nearest non-empty neighbours */
            struct {
                pfxtab_neigh prev, next;
            };
        };

        /** Number of associated suffixes
         *
         * A value of `(pfxtab_count) -1` indicates an "edge prefix", meaning
         * that there is no lower (resp. higher) prefix value with an
         * associated suffix in the ecurve. This can be checked with
         * ECURVE_ISEDGE()
         */
        pfxtab_count count;
    }
#if HAVE___ATTRIBUTE__
    __attribute__ ((packed))
#endif
    /** Table of prefixes
     *
     * Will be allocated to hold `#UPROC_PREFIX_MAX + 1` objects
     */
    *prefixes;

    /** Last non-empty prefix
     *
     * Needed by uproc_ecurve_add_prefix().
     *
     */
    uproc_prefix last_nonempty;

    /** `mmap()` file descriptor
     *
     * The underlying file descriptor if the ecurve is `mmap()ed` or -1.
     */
    int mmap_fd;

    /** `mmap()`ed memory region */
    void *mmap_ptr;

    /** Size of the `mmap()`ed region */
    size_t mmap_size;
};

#endif

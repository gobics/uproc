#ifndef UPRO_ECURVE_H
#define UPRO_ECURVE_H

/** \file upro/ecurve.h
 *
 * Look up amino acid words in an ecurve
 */

#include <stdio.h>
#include "upro/word.h"

/** Lookup return codes */
enum {
    /** Exact match */
    UPRO_LOOKUP_EXACT,

    /** No exact match */
    UPRO_LOOKUP_INEXACT,

    /** Out of bounds */
    UPRO_LOOKUP_OOB,
};

#define UPRO_ECURVE_EDGE ((size_t) -1)
#define UPRO_ECURVE_ISEDGE(p) ((p).count == UPRO_ECURVE_EDGE)

/** Struct defining an ecurve */
struct upro_ecurve {
    /** Translation alphabet */
    struct upro_alphabet alphabet;

    /** Number of suffixes */
    size_t suffix_count;

    /** Table of suffixes
     *
     * Will be allocated to hold `#suffix_count` objects
     */
    upro_suffix *suffixes;

    /** Table of families associated with the suffixes
     *
     * Will be allocated to hold `#suffix_count` objects
     */
    upro_family *families;

    /** Table that maps prefixes to entries in the ecurve's suffix table */
    struct upro_ecurve_pfxtable {
        /** Index of the first associated entry in #suffixes
         *
         * If there is no entry for the given prefix, this member contains
         * `#first + #count - 1` of the last "non-empty" prefix (equal to
         * `#first - 1` of the next non-empty one). In case of an "edge prefix"
         * the value is either `0` or `#suffix_count - 1`.
         */
        size_t first;

        /** Number of associated suffixes
         *
         * A value of `(size_t) -1` indicates an "edge prefix", meaning that
         * there is no lower (resp. higher) prefix value with an associated
         * suffix in the ecurve.
         */
        size_t count;
    }
    /** Table of prefixes
     *
     * Will be allocated to hold `#UPRO_PREFIX_MAX + 1` objects
     */
    *prefixes;

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

/** Initialize an empty ecurve object
 *
 * \param ecurve        ecurve obejct to initialize
 * \param alphabet      string to initialize the ecurve's alphabet (see upro_alphabet_init())
 * \param suffix_count  number of entries in the suffix table
 *
 * \retval #UPRO_SUCCESS  ecurve was initialized successfully
 * \retval #UPRO_ENOMEM   memory allocation failed
 */
int upro_ecurve_init(struct upro_ecurve *ecurve, const char *alphabet,
                   size_t suffix_count);

/** Free memory of an ecurve object
 *
 * \param ecurve    ecurve to destroy
 */
void upro_ecurve_destroy(struct upro_ecurve *ecurve);

/** Obtain translation alphabet
 *
 * Copies an ecurves internal alphabet into the provided #upro_alphabet pointer
 *
 * \param ecurve    an ecurve object
 * \param alpha     _OUT_: alphabet of `ecurve`
 */
void upro_ecurve_get_alphabet(const struct upro_ecurve *ecurve,
                            struct upro_alphabet *alpha);

/* Append one partial ecurve to another
 *
 * They may not overlap, i.e. the last non-empty prefix of `dest` needs to be
 * less than the first non-empty prefix of `src`.
 *
 * `dest` may not be "mmap()ed"
 *
 * \param dest  ecurve to append to
 * \param src   ecurve to append
 *
 * \retval #UPRO_EINVAL   `dest` is mmap()ed or `dest` and `src` overlap
 * \retval #UPRO_ENOMEM   memory allocation failed
 * \retval #UPRO_SUCCESS  else
 */
int upro_ecurve_append(struct upro_ecurve *dest, const struct upro_ecurve *src);

/** Find the closest neighbours of a word in the ecurve
 *
 * NOTE: `ecurve` may not be empty.
 *
 * If `word` was found exactly as it it in the ecurve, `#UPRO_LOOKUP_EXACT` will
 * be returned; if `word` is "outside" of the ecurve, `#UPRO_LOOKUP_OOB` will
 * be returned.  In both cases, the objects pointed to by `upper_neighbour` and
 * `upper_class` will be set equal to their `lower_*` counterparts.
 *
 * In case of an inexact match, `#UPRO_LOOKUP_INEXACT` will be returned, `lower_*`
 * and their `upper_*` counterparts might or might not differ.
 *
 * \param ecurve            ecurve object
 * \param word              word to search
 * \param lower_neighbour   _OUT_: lower neighbour word
 * \param lower_class       _OUT_: class of the lower neighbour
 * \param upper_neighbour   _OUT_: upper neighbour word
 * \param upper_class       _OUT_: class of the upper neighbour
 *
 * \return `#UPRO_LOOKUP_EXACT`, `#UPRO_LOOKUP_OOB` or `#UPRO_LOOKUP_INEXACT` as
 * described above.
 */
int upro_ecurve_lookup(const struct upro_ecurve *ecurve, const struct upro_word *word,
                     struct upro_word *lower_neighbour, upro_family *lower_class,
                     struct upro_word *upper_neighbour, upro_family *upper_class);

#endif

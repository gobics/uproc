#ifndef EC_ECURVE_H
#define EC_ECURVE_H

/** \file ecurve/ecurve.h
 *
 * Look up amino acid words in an ecurve
 */

#include <stdio.h>
#include "ecurve/word.h"

/** Lookup return codes */
enum {
    /** Exact match */
    EC_LOOKUP_EXACT,

    /** No exact match */
    EC_LOOKUP_INEXACT,

    /** Out of bounds */
    EC_LOOKUP_OOB,
};

/** Struct defining an ecurve */
struct ec_ecurve {
    /** Translation alphabet */
    struct ec_alphabet alphabet;

    /** Number of suffixes */
    size_t suffix_count;

    /** Table of suffixes
     *
     * Will be allocated to hold `#suffix_count` objects
     */
    ec_suffix *suffixes;

    /** Table of classes associated with the suffixes
     *
     * Will be allocated to hold `#suffix_count` objects
     */
    ec_class *classes;

    /** Table that maps prefixes to entries in the ecurve's suffix table */
    struct ec_ecurve_pfxtable {
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
     * Will be allocated to hold `#EC_PREFIX_MAX + 1` objects
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
 * \param alphabet      string to initialize the ecurve's alphabet (see ec_alphabet_init())
 * \param suffix_count  number of entries in the suffix table
 *
 * \retval #EC_SUCCESS  ecurve was initialized successfully
 * \retval #EC_ENOMEM   memory allocation failed
 */
int ec_ecurve_init(struct ec_ecurve *ecurve, const char *alphabet,
                   size_t suffix_count);

/** Free memory of an ecurve object
 *
 * \param ecurve    ecurve to destroy
 */
void ec_ecurve_destroy(struct ec_ecurve *ecurve);

/** Obtain translation alphabet
 *
 * Copies an ecurves internal alphabet into the provided #ec_alphabet pointer
 *
 * \param ecurve    an ecurve object
 * \param alpha     _OUT_: alphabet of `ecurve`
 */
void ec_ecurve_get_alphabet(const struct ec_ecurve *ecurve,
                            struct ec_alphabet *alpha);

/** Find the closest neighbours of a word in the ecurve
 *
 * NOTE: `ecurve` may not be empty.
 *
 * If `word` was found exactly as it it in the ecurve, `#EC_LOOKUP_EXACT` will
 * be returned; if `word` is "outside" of the ecurve, `#EC_LOOKUP_OOB` will
 * be returned.  In both cases, the objects pointed to by `upper_neighbour` and
 * `upper_class` will be set equal to their `lower_*` counterparts.
 *
 * In case of an inexact match, `#EC_LOOKUP_INEXACT` will be returned, `lower_*`
 * and their `upper_*` counterparts might or might not differ.
 *
 * \param ecurve            ecurve object
 * \param word              word to search
 * \param lower_neighbour   _OUT_: lower neighbour word
 * \param lower_class       _OUT_: class of the lower neighbour
 * \param upper_neighbour   _OUT_: upper neighbour word
 * \param upper_class       _OUT_: class of the upper neighbour
 *
 * \return `#EC_LOOKUP_EXACT`, `#EC_LOOKUP_OOB` or `#EC_LOOKUP_INEXACT` as
 * described above.
 */
int ec_ecurve_lookup(const struct ec_ecurve *ecurve, const struct ec_word *word,
                     struct ec_word *lower_neighbour, ec_class *lower_class,
                     struct ec_word *upper_neighbour, ec_class *upper_class);

#endif

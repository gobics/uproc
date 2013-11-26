/** \file uproc/ecurve.h
 *
 * Look up protein sequences from an "Evolutionary Curve"
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

#ifndef UPROC_ECURVE_H
#define UPROC_ECURVE_H

#include <stdio.h>
#include "uproc/word.h"

/** Lookup return codes */
enum {
    /** Exact match */
    UPROC_ECURVE_EXACT,

    /** No exact match */
    UPROC_ECURVE_INEXACT,

    /** Out of bounds */
    UPROC_ECURVE_OOB,
};

#define UPROC_ECURVE_EDGE ((size_t) -1)
#define UPROC_ECURVE_ISEDGE(p) ((p).count == UPROC_ECURVE_EDGE)

/** Struct defining an ecurve */
struct uproc_ecurve {
    /** Translation alphabet */
    struct uproc_alphabet alphabet;

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
     * Will be allocated to hold `#UPROC_PREFIX_MAX + 1` objects
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
 * \param alphabet      string to initialize the ecurve's alphabet (see uproc_alphabet_init())
 * \param suffix_count  number of entries in the suffix table
 *
 * \retval #UPROC_SUCCESS  ecurve was initialized successfully
 * \retval #UPROC_ENOMEM   memory allocation failed
 */
int uproc_ecurve_init(struct uproc_ecurve *ecurve, const char *alphabet,
                   size_t suffix_count);

/** Free memory of an ecurve object
 *
 * \param ecurve    ecurve to destroy
 */
void uproc_ecurve_destroy(struct uproc_ecurve *ecurve);

/** Obtain translation alphabet
 *
 * Copies an ecurves internal alphabet into the provided #uproc_alphabet pointer
 *
 * \param ecurve    an ecurve object
 * \param alpha     _OUT_: alphabet of `ecurve`
 */
void uproc_ecurve_get_alphabet(const struct uproc_ecurve *ecurve,
                            struct uproc_alphabet *alpha);

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
 * \retval #UPROC_EINVAL   `dest` is mmap()ed or `dest` and `src` overlap
 * \retval #UPROC_ENOMEM   memory allocation failed
 * \retval #UPROC_SUCCESS  else
 */
int uproc_ecurve_append(struct uproc_ecurve *dest, const struct uproc_ecurve *src);

/** Find the closest neighbours of a word in the ecurve
 *
 * NOTE: `ecurve` may not be empty.
 *
 * If `word` was found exactly as it it in the ecurve, `#UPROC_ECURVE_EXACT` will
 * be returned; if `word` is "outside" of the ecurve, `#UPROC_ECURVE_OOB` will
 * be returned.  In both cases, the objects pointed to by `upper_neighbour` and
 * `upper_class` will be set equal to their `lower_*` counterparts.
 *
 * In case of an inexact match, `#UPROC_ECURVE_INEXACT` will be returned, `lower_*`
 * and their `upper_*` counterparts might or might not differ.
 *
 * \param ecurve            ecurve object
 * \param word              word to search
 * \param lower_neighbour   _OUT_: lower neighbour word
 * \param lower_class       _OUT_: class of the lower neighbour
 * \param upper_neighbour   _OUT_: upper neighbour word
 * \param upper_class       _OUT_: class of the upper neighbour
 *
 * \return `#UPROC_ECURVE_EXACT`, `#UPROC_ECURVE_OOB` or `#UPROC_ECURVE_INEXACT` as
 * described above.
 */
int uproc_ecurve_lookup(const struct uproc_ecurve *ecurve, const struct uproc_word *word,
                     struct uproc_word *lower_neighbour, uproc_family *lower_class,
                     struct uproc_word *upper_neighbour, uproc_family *upper_class);

#endif

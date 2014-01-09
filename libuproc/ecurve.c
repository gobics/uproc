/* Look up protein sequences from an "Evolutionary Curve"
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

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/ecurve.h"
#include "uproc/word.h"

#include "ecurve_internal.h"

/** Perform a lookup in the prefix table.
 *
 * \param table         table of prefixes
 * \param key           prefix to search
 *
 * \return
 * #UPROC_ECURVE_OOB if `key` was less than the first (or greater than the
 * last) prefix that has at least one suffix associated, #UPROC_ECURVE_EXACT in
 * case of an exact match, else #UPROC_ECURVE_INEXACT.
 */
static int
prefix_lookup(const struct uproc_ecurve_pfxtable *table,
              uproc_prefix key, size_t *index, size_t *count,
              uproc_prefix *lower_prefix, uproc_prefix *upper_prefix)
{
    uproc_prefix tmp;

    /* outside of the "edge" */
    if (ECURVE_ISEDGE(table[key])) {
        /* below the first prefix that has an entry */
        if (!table[key].first) {
            for (tmp = key;
                 tmp < UPROC_PREFIX_MAX && ECURVE_ISEDGE(table[tmp]);
                 tmp++)
            {
                ;
            }
            *index = 0;
        }
        /* above the last prefix */
        else {
            for (tmp = key; tmp > 0 && ECURVE_ISEDGE(table[tmp]); tmp--) {
                ;
            }
            *index = table[tmp].first + table[tmp].count - 1;
        }

        *count = 1;
        *lower_prefix = *upper_prefix = tmp;
        return UPROC_ECURVE_OOB;
    }

    *index = table[key].first;

    /* inexact match, walk outward to find the closest non-empty neighbour
     * prefixes */
    if (table[key].count == 0) {
        *count = 2;
        for (tmp = key; tmp > 0 && !table[tmp].count; tmp--) {
            ;
        }
        *lower_prefix = tmp;
        for (tmp = key; tmp < UPROC_PREFIX_MAX && !table[tmp].count; tmp++) {
            ;
        }
        *upper_prefix = tmp;
        return UPROC_ECURVE_INEXACT;
    }

    *count = table[key].count;
    *lower_prefix = *upper_prefix = key;
    return UPROC_ECURVE_EXACT;
}


/** Find exact match or nearest neighbours in suffix array.
 *
 * If `key` is less than the first item in `search` (resp. greater than the
 * last one), #UPROC_ECURVE_OOB is returned and both `*lower` and `*upper` are
 * set to `0` (resp. `n - 1`).
 * If an exact match is found, returns #UPROC_ECURVE_EXACT and sets `*lower` and
 * `*upper` to the index of `key`.
 * Else, #UPROC_ECURVE_INEXACT is returned and `*lower` and `*upper` are set to
 * the indices of the values that are closest to `key`, i.e. such that
 * `search[*lower] < key < search[*upper]`.
 *
 * \param search    array to search
 * \param n         number of elements in `search`
 * \param key       value to find
 * \param lower     OUT: index of the lower neighbour
 * \param upper     OUT: index of the upper neighbour
 *
 * \return `#UPROC_ECURVE_EXACT`, `#UPROC_ECURVE_OOB` or
 * `#UPROC_ECURVE_INEXACT` as described above.
 */
static int
suffix_lookup(const uproc_suffix *search, size_t n, uproc_suffix key,
              size_t *lower, size_t *upper)
{
    size_t lo = 0, mid, hi = n - 1;

    if (!n || key < search[0]) {
        *lower = *upper = 0;
        return UPROC_ECURVE_OOB;
    }

    if (key > search[n - 1]) {
        *lower = *upper = n - 1;
        return UPROC_ECURVE_OOB;
    }

    while (hi > lo + 1) {
        mid = (hi + lo) / 2;

        if (key == search[mid]) {
            lo = mid;
            break;
        }
        else if (key > search[mid]) {
            lo = mid;
        }
        else {
            hi = mid;
        }
    }
    if (search[lo] == key) {
        hi = lo;
    }
    else if (search[hi] == key) {
        lo = hi;
    }
    *lower = lo;
    *upper = hi;
    return (lo == hi) ? UPROC_ECURVE_EXACT : UPROC_ECURVE_INEXACT;
}


uproc_ecurve *
uproc_ecurve_create(const char *alphabet, size_t suffix_count)
{
    struct uproc_ecurve_s *ec;
    if (suffix_count > PFXTAB_SUFFIX_MAX) {
        uproc_error_msg(UPROC_EINVAL, "too many suffixes");
        return NULL;
    }
    ec = malloc(sizeof *ec);
    if (!ec) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    ec->alphabet = uproc_alphabet_create(alphabet);
    if (!ec->alphabet) {
        return NULL;
    }

    ec->prefixes = malloc(
        sizeof *ec->prefixes * (UPROC_PREFIX_MAX + 1));
    if (!ec->prefixes) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }

    if (suffix_count) {
        ec->suffixes = malloc(sizeof *ec->suffixes * suffix_count);
        ec->families = malloc(sizeof *ec->families * suffix_count);
        if (!ec->suffixes || !ec->families) {
            uproc_ecurve_destroy(ec);
            uproc_error(UPROC_ENOMEM);
            return NULL;
        }
    }
    else {
        ec->suffixes = NULL;
        ec->families = NULL;
    }
    ec->suffix_count = suffix_count;

    ec->mmap_fd = -1;
    ec->mmap_ptr = NULL;
    ec->mmap_size = 0;
    return ec;
}


void
uproc_ecurve_destroy(uproc_ecurve *ecurve)
{
    uproc_alphabet_destroy(ecurve->alphabet);
    if (ecurve->mmap_fd > -1) {
        uproc_ecurve_munmap(ecurve);
    }
    else {
        free(ecurve->prefixes);
        free(ecurve->suffixes);
        free(ecurve->families);
    }
    free(ecurve);
}


int
uproc_ecurve_append(uproc_ecurve *dest, const uproc_ecurve *src)
{
    void *tmp;
    size_t new_suffix_count;
    uproc_prefix dest_last, src_first, p;

    if (dest->mmap_ptr) {
        return uproc_error_msg(UPROC_EINVAL, "can't append to mmap()ed ecurve");
    }

    for (dest_last = UPROC_PREFIX_MAX; dest_last > 0; dest_last--) {
        if (!ECURVE_ISEDGE(dest->prefixes[dest_last])) {
            break;
        }
    }
    for (src_first = 0; src_first < UPROC_PREFIX_MAX; src_first++) {
        if (!ECURVE_ISEDGE(src->prefixes[src_first])) {
            break;
        }
    }
    /* must not overlap! */
    if (src_first <= dest_last) {
        return uproc_error_msg(UPROC_EINVAL, "overlapping ecurves");
    }

    new_suffix_count = dest->suffix_count + src->suffix_count;
    if (new_suffix_count > PFXTAB_SUFFIX_MAX) {
        return uproc_error_msg(UPROC_EINVAL, "too many suffixes");
    }
    tmp = realloc(dest->suffixes, new_suffix_count * sizeof *dest->suffixes);
    if (!tmp) {
        return uproc_error(UPROC_ENOMEM);
    }
    dest->suffixes = tmp;

    tmp = realloc(dest->families, new_suffix_count * sizeof *dest->families);
    if (!tmp) {
        return uproc_error(UPROC_ENOMEM);
    }
    dest->families = tmp;

    memcpy(dest->suffixes + dest->suffix_count,
            src->suffixes,
            src->suffix_count * sizeof *src->suffixes);
    memcpy(dest->families + dest->suffix_count,
            src->families,
            src->suffix_count * sizeof *src->families);

    for (p = dest_last + 1; p < src_first; p++) {
        dest->prefixes[p].count = 0;
        dest->prefixes[p].first = dest->suffix_count - 1;
    }
    for (p = src_first; p <= UPROC_PREFIX_MAX; p++) {
        dest->prefixes[p].first = src->prefixes[p].first + dest->suffix_count;
        dest->prefixes[p].count = src->prefixes[p].count;
    }
    dest->suffix_count = new_suffix_count;

    return 0;
}


int
uproc_ecurve_lookup(const uproc_ecurve *ecurve,
                    const struct uproc_word *word,
                    struct uproc_word *lower_neighbour,
                    uproc_family *lower_class,
                    struct uproc_word *upper_neighbour,
                    uproc_family *upper_class)
{
    int res;
    uproc_prefix p_lower, p_upper;
    size_t index, count;
    size_t lower, upper;

    res = prefix_lookup(ecurve->prefixes, word->prefix, &index, &count,
                        &p_lower, &p_upper);

    if (res == UPROC_ECURVE_EXACT) {
        res = suffix_lookup(&ecurve->suffixes[index], count, word->suffix,
                            &lower, &upper);
        if (res != UPROC_ECURVE_EXACT) {
            res = UPROC_ECURVE_INEXACT;
        }
    }
    else {
        lower = 0;
        upper = (res == UPROC_ECURVE_OOB) ? 0 : 1;
    }

    /* `lower` and `upper` are relative to `index` */
    lower += index;
    upper += index;

    /* populate output variables */
    lower_neighbour->prefix = p_lower;
    lower_neighbour->suffix = ecurve->suffixes[lower];
    *lower_class = ecurve->families[lower];
    upper_neighbour->prefix = p_upper;
    upper_neighbour->suffix = ecurve->suffixes[upper];
    *upper_class = ecurve->families[upper];

    return res;
}

uproc_alphabet *
uproc_ecurve_alphabet(const uproc_ecurve *ecurve)
{
    return ecurve->alphabet;
}

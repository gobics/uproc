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
#include "uproc/mmap.h"

/** Perform a lookup in the prefix table.
 *
 * \param table         table of prefixes
 * \param key           prefix to search
 * \param lower_prefix  OUT: closest lower prefix (`key` in case of an exact match)
 * \param lower_suffix  OUT: pointer to the lower neighbouring suffix table entry
 * \param upper_prefix  OUT: closest lower prefix (`key` in case of an exact match)
 * \param upper_suffix  OUT: pointer to the lower neighbouring suffix table entry
 *
 * \return
 * #UPROC_ECURVE_OOB if `key` was less than the first (or greater than the last)
 * prefix that has at least one suffix associated, #UPROC_ECURVE_EXACT in case of
 * an exact match, else #UPROC_ECURVE_INEXACT.
 */
static int prefix_lookup(const struct uproc_ecurve_pfxtable *table,
                         uproc_prefix key, size_t *index, size_t *count,
                         uproc_prefix *lower_prefix, uproc_prefix *upper_prefix);

/** Find exact match or nearest neighbours in suffix array.
 *
 * If `key` is less than the first item in `search` (resp. greater than the
 * last one), #UPROC_ECURVE_OOB is returned and both `*lower` and `*upper` are set to
 * `0` (resp. `n - 1`).
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
 * \return `#UPROC_ECURVE_EXACT`, `#UPROC_ECURVE_OOB` or `#UPROC_ECURVE_INEXACT` as
 * described above.
 */
static int suffix_lookup(const uproc_suffix *search, size_t n, uproc_suffix key,
                         size_t *lower, size_t *upper);

int
uproc_ecurve_init(struct uproc_ecurve *ecurve, const char *alphabet,
               size_t suffix_count)
{
    int res;

    res = uproc_alphabet_init(&ecurve->alphabet, alphabet);
    if (res) {
        return res;
    }

    ecurve->prefixes = malloc(sizeof *ecurve->prefixes * (UPROC_PREFIX_MAX + 1));
    if (!ecurve->prefixes) {
        return uproc_error(UPROC_ENOMEM);
    }

    if (suffix_count) {
        ecurve->suffixes = malloc(sizeof *ecurve->suffixes * suffix_count);
        ecurve->families = malloc(sizeof *ecurve->families * suffix_count);
        if (!ecurve->suffixes || !ecurve->families) {
            uproc_ecurve_destroy(ecurve);
            return uproc_error(UPROC_ENOMEM);
        }
    }
    else {
        ecurve->suffixes = NULL;
        ecurve->families = NULL;
    }
    ecurve->suffix_count = suffix_count;

    ecurve->mmap_fd = -1;
    ecurve->mmap_ptr = NULL;
    ecurve->mmap_size = 0;
    return 0;
}

void
uproc_ecurve_destroy(struct uproc_ecurve *ecurve)
{
    if (ecurve->mmap_fd > -1) {
        uproc_mmap_unmap(ecurve);
        return;
    }
    free(ecurve->prefixes);
    free(ecurve->suffixes);
    free(ecurve->families);
}

int
uproc_ecurve_append(struct uproc_ecurve *dest, const struct uproc_ecurve *src)
{
    void *tmp;
    size_t new_suffix_count;
    uproc_prefix dest_last, src_first, p;

    if (dest->mmap_ptr) {
        return uproc_error_msg(UPROC_EINVAL, "can't append to mmap()ed ecurve");
    }

    for (dest_last = UPROC_PREFIX_MAX; dest_last > 0; dest_last--) {
        if (!UPROC_ECURVE_ISEDGE(dest->prefixes[dest_last])) {
            break;
        }
    }
    for (src_first = 0; src_first < UPROC_PREFIX_MAX; src_first++) {
        if (!UPROC_ECURVE_ISEDGE(src->prefixes[src_first])) {
            break;
        }
    }
    /* must not overlap! */
    if (src_first <= dest_last) {
        return uproc_error_msg(UPROC_EINVAL, "overlapping ecurves");
    }

    new_suffix_count = dest->suffix_count + src->suffix_count;
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

void uproc_ecurve_get_alphabet(const struct uproc_ecurve *ecurve,
                            struct uproc_alphabet *alpha)
{
    *alpha = ecurve->alphabet;
}

int
uproc_ecurve_lookup(const struct uproc_ecurve *ecurve, const struct uproc_word *word,
                 struct uproc_word *lower_neighbour, uproc_family *lower_class,
                 struct uproc_word *upper_neighbour, uproc_family *upper_class)
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
        res = (res == UPROC_ECURVE_EXACT) ? UPROC_ECURVE_EXACT : UPROC_ECURVE_INEXACT;
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

static int
prefix_lookup(const struct uproc_ecurve_pfxtable *table,
              uproc_prefix key, size_t *index, size_t *count,
              uproc_prefix *lower_prefix, uproc_prefix *upper_prefix)
{
    uproc_prefix tmp;

    /* outside of the "edge" */
    if (UPROC_ECURVE_ISEDGE(table[key])) {
        /* below the first prefix that has an entry */
        if (!table[key].first) {
            for (tmp = key; tmp < UPROC_PREFIX_MAX && UPROC_ECURVE_ISEDGE(table[tmp]); tmp++) {
                ;
            }
            *index = 0;
        }
        /* above the last prefix */
        else {
            for (tmp = key; tmp > 0 && UPROC_ECURVE_ISEDGE(table[tmp]); tmp--) {
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

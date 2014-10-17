/* Look up protein sequences from an "Evolutionary Curve"
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

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/ecurve.h"
#include "uproc/word.h"
#include "uproc/list.h"

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
static int prefix_lookup(const struct uproc_ecurve_pfxtable *table,
                         uproc_prefix key, size_t *index, size_t *count,
                         uproc_prefix *lower_prefix, uproc_prefix *upper_prefix)
{
    uproc_prefix tmp;

    /* outside of the "edge" */
    if (ECURVE_ISEDGE(table[key])) {
        /* below the first prefix that has an entry */
        if (!table[key].prev) {
            for (tmp = key;
                 tmp < UPROC_PREFIX_MAX && ECURVE_ISEDGE(table[tmp]);) {
                tmp += table[tmp].next;
            }
            *index = 0;
        }
        /* above the last prefix */
        else {
            for (tmp = key; tmp > 0 && ECURVE_ISEDGE(table[tmp]);) {
                tmp -= table[tmp].prev;
            }
            *index = table[tmp].first + table[tmp].count - 1;
        }

        *count = 1;
        *lower_prefix = *upper_prefix = tmp;
        return UPROC_ECURVE_OOB;
    }

    /* inexact match, walk outward to find the closest non-empty neighbour
     * prefixes */
    if (table[key].count == 0) {
        *count = 2;
        for (tmp = key; tmp > 0 && !table[tmp].count;) {
            tmp -= table[tmp].prev;
        }
        *index = table[tmp].first + table[tmp].count - 1;
        *lower_prefix = tmp;
        for (tmp = key; tmp < UPROC_PREFIX_MAX && !table[tmp].count;) {
            tmp += table[tmp].next;
        }
        *upper_prefix = tmp;
        return UPROC_ECURVE_INEXACT;
    }

    *index = table[key].first;

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
static int suffix_lookup(const uproc_suffix *search, size_t n, uproc_suffix key,
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
        } else if (key > search[mid]) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    if (search[lo] == key) {
        hi = lo;
    } else if (search[hi] == key) {
        lo = hi;
    }
    *lower = lo;
    *upper = hi;
    return (lo == hi) ? UPROC_ECURVE_EXACT : UPROC_ECURVE_INEXACT;
}

static inline pfxtab_neigh neigh_dist(uproc_prefix a, uproc_prefix b)
{
    uintmax_t dist;
    if (a > b) {
        dist = a - b;
    } else {
        dist = b - a;
    }
    return dist < PFXTAB_NEIGH_MAX ? dist : PFXTAB_NEIGH_MAX;
}

static inline int ecurve_realloc(struct uproc_ecurve_s *ec, size_t suffix_count)
{
    void *tmp;
    size_t alloc;
    if (suffix_count < ec->suffix_alloc) {
        ec->suffix_count = suffix_count;
        return 0;
    }

    alloc = ec->suffix_alloc;
    if (!alloc) {
        alloc = 1 << 16;
    }
    while (alloc < suffix_count) {
        alloc = ec->suffix_alloc * 2;
    }

    tmp = realloc(ec->suffixes, sizeof *ec->suffixes * alloc);
    if (!tmp) {
        return uproc_error(UPROC_ENOMEM);
    }
    ec->suffixes = tmp;

    tmp = realloc(ec->families, sizeof *ec->families * alloc);
    if (!tmp) {
        return uproc_error(UPROC_ENOMEM);
    }
    ec->families = tmp;

    ec->suffix_alloc = alloc;
    ec->suffix_count = suffix_count;
    return 0;
}

uproc_ecurve *uproc_ecurve_create(const char *alphabet, size_t suffix_count)
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
    *ec = (struct uproc_ecurve_s){0};

    ec->alphabet = uproc_alphabet_create(alphabet);
    if (!ec->alphabet) {
        uproc_ecurve_destroy(ec);
        return NULL;
    }

    ec->prefixes = malloc(sizeof *ec->prefixes * (UPROC_PREFIX_MAX + 1));
    if (!ec->prefixes) {
        uproc_ecurve_destroy(ec);
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
    } else {
        ec->suffixes = NULL;
        ec->families = NULL;
    }
    ec->suffix_count = suffix_count;

    ec->mmap_fd = -1;
    ec->mmap_ptr = NULL;
    ec->mmap_size = 0;
    return ec;
}

void uproc_ecurve_destroy(uproc_ecurve *ecurve)
{
    if (!ecurve) {
        return;
    }

    uproc_alphabet_destroy(ecurve->alphabet);
    if (ecurve->mmap_fd > -1) {
        uproc_ecurve_munmap(ecurve);
    } else {
        free(ecurve->prefixes);
        free(ecurve->suffixes);
        free(ecurve->families);
    }
    free(ecurve);
}

int uproc_ecurve_add_prefix(uproc_ecurve *ecurve, uproc_prefix pfx,
                            uproc_list *suffixes)
{
    int res;
    uproc_prefix p;
    struct uproc_ecurve_pfxtable *pt;
    size_t suffix_count, old_suffix_count;

    if (!uproc_list_size(suffixes)) {
        return uproc_error_msg(UPROC_EINVAL, "empty suffix list");
    }

    if (uproc_list_size(suffixes) >= ECURVE_EDGE) {
        return uproc_error_msg(UPROC_EINVAL, "too many suffixes");
    }

    if (pfx <= ecurve->last_nonempty && ecurve->suffix_count) {
        return uproc_error_msg(UPROC_EINVAL,
                               "new prefix must be greater than last nonempty");
    }

    for (p = ecurve->last_nonempty + !!ecurve->suffix_count; p < pfx; p++) {
        pt = &ecurve->prefixes[p];
        /* empty ecurve -> mark leading prefixes as "edge" */
        if (!ecurve->suffix_count) {
            pt->prev = 0;
            pt->next = neigh_dist(p, pfx);
            pt->count = ECURVE_EDGE;
        } else {
            pt->prev = neigh_dist(ecurve->last_nonempty, p);
            pt->next = neigh_dist(p, pfx);
            pt->count = 0;
        }
    }

    suffix_count = uproc_list_size(suffixes);
    pt = &ecurve->prefixes[pfx];
    pt->first = ecurve->suffix_count;
    pt->count = suffix_count;

    old_suffix_count = ecurve->suffix_count;
    res = ecurve_realloc(ecurve, ecurve->suffix_count + suffix_count);
    if (res) {
        return res;
    }

    for (size_t i = 0; i < suffix_count; i++) {
        struct uproc_ecurve_suffixentry entry;
        uproc_list_get(suffixes, i, &entry);
        ecurve->suffixes[old_suffix_count + i] = entry.suffix;
        ecurve->families[old_suffix_count + i] = entry.family;
    }

    ecurve->last_nonempty = pfx;
    return 0;
}

int uproc_ecurve_finalize(uproc_ecurve *ecurve)
{
    uproc_prefix p;
    struct uproc_ecurve_pfxtable *pt;
    for (p = ecurve->last_nonempty + 1; p <= UPROC_PREFIX_MAX; p++) {
        pt = &ecurve->prefixes[p];
        pt->prev = neigh_dist(ecurve->last_nonempty, p);
        pt->next = 0;
        pt->count = ECURVE_EDGE;
    }
    void *tmp;
    tmp = realloc(ecurve->suffixes,
                  sizeof *ecurve->suffixes * ecurve->suffix_count);
    if (!tmp) {
        return uproc_error(UPROC_ENOMEM);
    }
    ecurve->suffixes = tmp;

    tmp = realloc(ecurve->families,
                  sizeof *ecurve->families * ecurve->suffix_count);
    if (!tmp) {
        return uproc_error(UPROC_ENOMEM);
    }
    ecurve->families = tmp;
    return 0;
}

int uproc_ecurve_lookup(const uproc_ecurve *ecurve,
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
    } else {
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

uproc_alphabet *uproc_ecurve_alphabet(const uproc_ecurve *ecurve)
{
    return ecurve->alphabet;
}

#include <stdlib.h>

#include "ecurve/common.h"
#include "ecurve/ecurve.h"
#include "ecurve/word.h"

#if HAVE_MMAP
#include "ecurve/mmap.h"
#endif

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
 * #EC_LOOKUP_OOB if `key` was less than the first (or greater than the last)
 * prefix that has at least one suffix associated, #EC_LOOKUP_EXACT in case of
 * an exact match, else #EC_LOOKUP_INEXACT.
 */
static int prefix_lookup(const struct ec_ecurve_pfxtable *table,
                         ec_prefix key, size_t *index, size_t *count,
                         ec_prefix *lower_prefix, ec_prefix *upper_prefix);

/** Find exact match or nearest neighbours in suffix array.
 *
 * If `key` is less than the first item in `search` (resp. greater than the
 * last one), #EC_LOOKUP_OOB is returned and both `*lower` and `*upper` are set to
 * `0` (resp. `n - 1`).
 * If an exact match is found, returns #EC_LOOKUP_EXACT and sets `*lower` and
 * `*upper` to the index of `key`.
 * Else, #EC_LOOKUP_INEXACT is returned and `*lower` and `*upper` are set to
 * the indices of the values that are closest to `key`, i.e. such that
 * `search[*lower] < key < search[*upper]`.
 *
 * \param search    array to search
 * \param n         number of elements in `search`
 * \param key       value to find
 * \param lower     OUT: index of the lower neighbour
 * \param upper     OUT: index of the upper neighbour
 *
 * \return `#EC_LOOKUP_EXACT`, `#EC_LOOKUP_OOB` or `#EC_LOOKUP_INEXACT` as
 * described above.
 */
static int suffix_lookup(const ec_suffix *search, size_t n, ec_suffix key,
                         size_t *lower, size_t *upper);

int
ec_ecurve_init(struct ec_ecurve *ecurve, const char *alphabet,
               size_t suffix_count)
{
    int res;
    size_t i;

    res = ec_alphabet_init(&ecurve->alphabet, alphabet);
    if (EC_ISERROR(res)) {
        return res;
    }

    ecurve->prefixes = malloc(sizeof *ecurve->prefixes * (EC_PREFIX_MAX + 1));
    if (!ecurve->prefixes) {
        return EC_ENOMEM;
    }
    for (i = 0; i <= EC_PREFIX_MAX; i++) {
        ecurve->prefixes[i].first = 0;
        ecurve->prefixes[i].count = 0;
    }

    if (suffix_count) {
        ecurve->suffixes = malloc(sizeof *ecurve->suffixes * suffix_count);
        ecurve->classes = malloc(sizeof *ecurve->classes * suffix_count);
        if (!ecurve->suffixes || !ecurve->classes) {
            ec_ecurve_destroy(ecurve);
            return EC_ENOMEM;
        }
    }
    else {
        ecurve->suffixes = NULL;
        ecurve->classes = NULL;
    }
    ecurve->suffix_count = suffix_count;

    ecurve->mmap_fd = -1;
    ecurve->mmap_ptr = NULL;
    ecurve->mmap_size = 0;
    return EC_SUCCESS;
}

void
ec_ecurve_destroy(struct ec_ecurve *ecurve)
{
#if HAVE_MMAP
    if (ecurve->mmap_fd > -1) {
        ec_mmap_unmap(ecurve);
        return;
    }
#endif
    free(ecurve->prefixes);
    free(ecurve->suffixes);
    free(ecurve->classes);
}

int
ec_ecurve_append(struct ec_ecurve *dest, const struct ec_ecurve *src)
{
    void *tmp;
    size_t new_suffix_count;
    ec_prefix dest_last, src_first, p;

    if (dest->mmap_ptr) {
        return EC_EINVAL;
    }

    for (dest_last = EC_PREFIX_MAX; dest_last > 0; dest_last--) {
        if (!EC_ECURVE_ISEDGE(dest->prefixes[dest_last])) {
            break;
        }
    }
    for (src_first = 0; src_first < EC_PREFIX_MAX; src_first++) {
        if (!EC_ECURVE_ISEDGE(src->prefixes[src_first])) {
            break;
        }
    }
    /* must not overlap! */
    if (src_first <= dest_last) {
        return EC_EINVAL;
    }

    new_suffix_count = dest->suffix_count + src->suffix_count;
    tmp = realloc(dest->suffixes, new_suffix_count * sizeof *dest->suffixes);
    if (!tmp) {
        return EC_ENOMEM;
    }
    dest->suffixes = tmp;

    tmp = realloc(dest->classes, new_suffix_count * sizeof *dest->classes);
    if (!tmp) {
        return EC_ENOMEM;
    }
    dest->classes = tmp;
    memcpy(dest->suffixes + dest->suffix_count,
            src->suffixes,
            src->suffix_count * sizeof *src->suffixes);
    memcpy(dest->classes + dest->suffix_count,
            src->classes,
            src->suffix_count * sizeof *src->classes);

    //->prefixes[dest_last].first = dest->suffix_count - 2;
    for (p = dest_last + 1; p < src_first; p++) {
        dest->prefixes[p].count = 0;
        dest->prefixes[p].first = dest->suffix_count - 1;
    }
    for (p = src_first; p <= EC_PREFIX_MAX; p++) {
        dest->prefixes[p].first = src->prefixes[p].first + dest->suffix_count;
        dest->prefixes[p].count = src->prefixes[p].count;
    }
    dest->suffix_count = new_suffix_count;

    return EC_SUCCESS;
}

void ec_ecurve_get_alphabet(const struct ec_ecurve *ecurve,
                            struct ec_alphabet *alpha)
{
    *alpha = ecurve->alphabet;
}

int
ec_ecurve_lookup(const struct ec_ecurve *ecurve, const struct ec_word *word,
                 struct ec_word *lower_neighbour, ec_class *lower_class,
                 struct ec_word *upper_neighbour, ec_class *upper_class)
{
    int res;
    ec_prefix p_lower, p_upper;
    size_t index, count;
    size_t lower, upper;

    res = prefix_lookup(ecurve->prefixes, word->prefix, &index, &count,
                        &p_lower, &p_upper);

    if (res == EC_LOOKUP_EXACT) {
        res = suffix_lookup(&ecurve->suffixes[index], count, word->suffix,
                            &lower, &upper);
        res = (res == EC_LOOKUP_EXACT) ? EC_LOOKUP_EXACT : EC_LOOKUP_INEXACT;
    }
    else {
        lower = 0;
        upper = (res == EC_LOOKUP_OOB) ? 0 : 1;
    }

    /* `lower` and `upper` are relative to `index` */
    lower += index;
    upper += index;

    /* populate output variables */
    lower_neighbour->prefix = p_lower;
    lower_neighbour->suffix = ecurve->suffixes[lower];
    *lower_class = ecurve->classes[lower];
    upper_neighbour->prefix = p_upper;
    upper_neighbour->suffix = ecurve->suffixes[upper];
    *upper_class = ecurve->classes[upper];

    return res;
}

static int
prefix_lookup(const struct ec_ecurve_pfxtable *table,
              ec_prefix key, size_t *index, size_t *count,
              ec_prefix *lower_prefix, ec_prefix *upper_prefix)
{
    ec_prefix tmp;

    /* outside of the "edge" */
    if (EC_ECURVE_ISEDGE(table[key])) {
        /* below the first prefix that has an entry */
        if (!table[key].first) {
            for (tmp = key; tmp < EC_PREFIX_MAX && EC_ECURVE_ISEDGE(table[tmp]); tmp++) {
                ;
            }
            *index = 0;
        }
        /* above the last prefix */
        else {
            for (tmp = key; tmp > 0 && EC_ECURVE_ISEDGE(table[tmp]); tmp--) {
                ;
            }
            *index = table[tmp].first + table[tmp].count - 1;
        }

        *count = 1;
        *lower_prefix = *upper_prefix = tmp;
        return EC_LOOKUP_OOB;
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
        for (tmp = key; tmp < EC_PREFIX_MAX && !table[tmp].count; tmp++) {
            ;
        }
        *upper_prefix = tmp;
        return EC_LOOKUP_INEXACT;
    }

    *count = table[key].count;
    *lower_prefix = *upper_prefix = key;
    return EC_LOOKUP_EXACT;
}

static int
suffix_lookup(const ec_suffix *search, size_t n, ec_suffix key,
              size_t *lower, size_t *upper)
{
    size_t lo = 0, mid, hi = n - 1;

    if (!n || key < search[0]) {
        *lower = *upper = 0;
        return EC_LOOKUP_OOB;
    }

    if (key > search[n - 1]) {
        *lower = *upper = n - 1;
        return EC_LOOKUP_OOB;
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
    return (lo == hi) ? EC_LOOKUP_EXACT : EC_LOOKUP_INEXACT;
}

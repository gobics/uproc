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
ec_ecurve_init(ec_ecurve *ecurve, const char *alphabet, size_t suffix_count)
{
    struct ec_ecurve_s *e = ecurve;
    size_t i;

    if (ec_alphabet_init(&e->alphabet, alphabet) != EC_SUCCESS) {
        return EC_FAILURE;
    }

    e->prefix_table = malloc(sizeof *e->prefix_table * (EC_PREFIX_MAX + 1));
    if (!e->prefix_table) {
        return EC_FAILURE;
    }
    for (i = 0; i <= EC_PREFIX_MAX; i++) {
        e->prefix_table[i].first = 0;
        e->prefix_table[i].count = 0;
    }

    if (suffix_count) {
        e->suffix_count = suffix_count;
        e->suffix_table = malloc(sizeof *e->suffix_table * suffix_count);
        e->class_table = malloc(sizeof *e->class_table * suffix_count);
        if (!e->suffix_table || !e->class_table) {
            ec_ecurve_destroy(e);
            return EC_FAILURE;
        }
    }

#if HAVE_MMAP
    e->mmap_fd = -1;
    e->mmap_ptr = NULL;
    e->mmap_size = 0;
#endif
    return EC_SUCCESS;
}

void
ec_ecurve_destroy(ec_ecurve *ecurve)
{
    struct ec_ecurve_s *e = ecurve;
#if HAVE_MMAP
    if (e->mmap_fd > -1) {
        ec_mmap_unmap(ecurve);
        return;
    }
#endif
    free(e->prefix_table);
    free(e->suffix_table);
    free(e->class_table);
}

void ec_ecurve_get_alphabet(const ec_ecurve *ecurve, ec_alphabet *alpha)
{
    *alpha = ecurve->alphabet;
}

int
ec_ecurve_lookup(const ec_ecurve *ecurve, const struct ec_word *word,
                 struct ec_word *lower_neighbour, ec_class *lower_class,
                 struct ec_word *upper_neighbour, ec_class *upper_class)
{
    int res;
    ec_prefix p_lower, p_upper;
    size_t index, count;
    size_t lower, upper;

    res = prefix_lookup(ecurve->prefix_table, word->prefix,
                        &index, &count, &p_lower, &p_upper);

    if (res == EC_LOOKUP_EXACT) {
        res = suffix_lookup(&ecurve->suffix_table[index], count,
                            word->suffix, &lower, &upper);
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
    lower_neighbour->suffix = ecurve->suffix_table[lower];
    *lower_class = ecurve->class_table[lower];
    upper_neighbour->prefix = p_upper;
    upper_neighbour->suffix = ecurve->suffix_table[upper];
    *upper_class = ecurve->class_table[upper];

    return res;
}

static int
prefix_lookup(const struct ec_ecurve_pfxtable *table,
              ec_prefix key, size_t *index, size_t *count,
              ec_prefix *lower_prefix, ec_prefix *upper_prefix)
{
    ec_prefix tmp;

    if (table[key].count == (size_t)-1) {
        if (!table[key].first) {
            for (tmp = key; tmp < EC_PREFIX_MAX && table[tmp].count == (size_t)-1; tmp++) {
                ;
            }
            *index = 0;
        }
        else {
            for (tmp = key; tmp > 0 && table[tmp].count == (size_t)-1; tmp--) {
                ;
            }
            *index = table[tmp].first + table[tmp].count - 1;
        }

        *count = 1;
        *lower_prefix = *upper_prefix = tmp;
        return EC_LOOKUP_OOB;
    }

    *index = table[key].first;

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

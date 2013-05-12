#include <stdlib.h>
#include "ecurve/common.h"
#include "ecurve/word.h"

/* Finds exact match or nearest neighbours in suffix array.
 *
 * If an exact match is found, returns 0 and sets `*lower` and `*upper` to the
 * value of `key`. 
 * If `key` is less than the first item in `search` (resp. greater than the
 * last one), -1 is returned and both `*lower` and `*upper` are set to the
 * value of the first (last) item.
 * Else, 1 is returned and `*lower` and `*upper` are set to the values that are
 * closest to `key`, with `*lower < key < *upper`.
 *
 * \param   key     value to find
 * \param   search  array to search
 * \param   n       number of elements
 * \param   lower   pointer to store "lower neighbour"
 * \param   upper   pointer to store "upper neighbour"
 *
 * \return  -1, 0 or 1; as described above.
 */
static int nbsearch(ec_suffix key, ec_suffix *search, size_t n,
                    ec_suffix *lower, ec_suffix *upper)
{
    size_t lo = 0, mid, hi = n - 1;

    if (key < search[0]) {
        *lower = *upper = search[0];
        return -1;
    }

    if (key > search[n - 1]) {
        *lower = *upper = search[n - 1];
        return -1;
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
    if (search[lo] == key || search[hi] == key) {
        *lower = *upper = key;
        return 0;
    }
    *lower = search[lo];
    *upper = search[hi];
    return 1;
}

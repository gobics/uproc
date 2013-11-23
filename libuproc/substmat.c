#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/alphabet.h"
#include "uproc/io.h"
#include "uproc/matrix.h"
#include "uproc/substmat.h"

#define UPROC_DISTMAT_INDEX(x, y) ((x) << UPROC_AMINO_BITS | (y))

int
uproc_substmat_init(struct uproc_substmat *mat)
{
    *mat = (struct uproc_substmat) { { { 0.0 } } };
    return 0;
}

double
uproc_substmat_get(const struct uproc_substmat *mat, unsigned pos, uproc_amino x,
        uproc_amino y)
{
    return mat->dists[pos][UPROC_DISTMAT_INDEX(x, y)];
}

void
uproc_substmat_set(struct uproc_substmat *mat, unsigned pos, uproc_amino x,
        uproc_amino y, double dist)
{
    mat->dists[pos][UPROC_DISTMAT_INDEX(x, y)] = dist;
}

int
uproc_substmat_loadv(struct uproc_substmat *mat, enum uproc_io_type iotype,
        const char *pathfmt, va_list ap)
{
    int res;
    size_t i, j, k, rows, cols, sz;
    struct uproc_matrix matrix;

    res = uproc_matrix_loadv(&matrix, iotype, pathfmt, ap);
    if (res) {
        return res;
    }

    uproc_matrix_dimensions(&matrix, &rows, &cols);
    sz = rows * cols;
#define SUBSTMAT_TOTAL (UPROC_SUFFIX_LEN * UPROC_ALPHABET_SIZE * UPROC_ALPHABET_SIZE)
    if (sz != SUBSTMAT_TOTAL) {
        res = uproc_error_msg(
            UPROC_EINVAL,
            "invalid substitution matrix (%zu elements; expected %zu)", sz,
            SUBSTMAT_TOTAL);
        goto error;
    }

    for (i = 0; i < UPROC_SUFFIX_LEN; i++) {
        for (j = 0; j < UPROC_ALPHABET_SIZE; j++) {
            for (k = 0; k < UPROC_ALPHABET_SIZE; k++) {
                /* treat `matrix` like a vector of length
                 *   UPROC_SUFFIX_LEN * UPROC_ALPHABET_SIZE * UPROC_ALPHABET_SIZE
                 * (this assumes uproc_matrix uses a linear representation)
                 */
                size_t idx = (i * UPROC_ALPHABET_SIZE + j) * UPROC_ALPHABET_SIZE + k;
                uproc_substmat_set(mat, i, k, j, uproc_matrix_get(&matrix, 0, idx));
            }
        }
    }
error:
    uproc_matrix_destroy(&matrix);
    return res;
}

int
uproc_substmat_load(struct uproc_substmat *mat, enum uproc_io_type iotype,
        const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_substmat_loadv(mat, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}

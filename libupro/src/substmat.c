#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "upro/common.h"
#include "upro/alphabet.h"
#include "upro/io.h"
#include "upro/matrix.h"
#include "upro/substmat.h"

#define UPRO_DISTMAT_INDEX(x, y) ((x) << UPRO_AMINO_BITS | (y))

int
upro_substmat_init(struct upro_substmat *mat)
{
    *mat = (struct upro_substmat) { { { 0.0 } } };
    return UPRO_SUCCESS;
}

double
upro_substmat_get(const struct upro_substmat *mat, unsigned pos, upro_amino x,
        upro_amino y)
{
    return mat->dists[pos][UPRO_DISTMAT_INDEX(x, y)];
}

void
upro_substmat_set(struct upro_substmat *mat, unsigned pos, upro_amino x,
        upro_amino y, double dist)
{
    mat->dists[pos][UPRO_DISTMAT_INDEX(x, y)] = dist;
}

int
upro_substmat_loadv(struct upro_substmat *mat, enum upro_io_type iotype,
        const char *pathfmt, va_list ap)
{
    int res;
    size_t i, j, k, rows, cols;
    struct upro_matrix matrix;

    res = upro_matrix_loadv(&matrix, iotype, pathfmt, ap);
    if (UPRO_ISERROR(res)) {
        return res;
    }

    upro_matrix_dimensions(&matrix, &rows, &cols);
    if (rows * cols != UPRO_SUFFIX_LEN * UPRO_ALPHABET_SIZE *
            UPRO_ALPHABET_SIZE) {
        res = UPRO_EINVAL;
        goto error;
    }

    for (i = 0; i < UPRO_SUFFIX_LEN; i++) {
        for (j = 0; j < UPRO_ALPHABET_SIZE; j++) {
            for (k = 0; k < UPRO_ALPHABET_SIZE; k++) {
                /* treat `matrix` like a vector of length
                 *   UPRO_SUFFIX_LEN * UPRO_ALPHABET_SIZE * UPRO_ALPHABET_SIZE
                 * (this assumes upro_matrix uses a linear representation)
                 */
                size_t idx = (i * UPRO_ALPHABET_SIZE + j) * UPRO_ALPHABET_SIZE + k;
                upro_substmat_set(mat, i, k, j, upro_matrix_get(&matrix, 0, idx));
            }
        }
    }
error:
    upro_matrix_destroy(&matrix);
    return res;
}

int
upro_substmat_load(struct upro_substmat *mat, enum upro_io_type iotype,
        const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = upro_substmat_loadv(mat, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}

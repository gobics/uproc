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
    *mat = (struct upro_substmat) { { 0.0 } };
    return UPRO_SUCCESS;
}

double
upro_substmat_get(const struct upro_substmat *mat, upro_amino x, upro_amino y)
{
    return mat->dists[UPRO_DISTMAT_INDEX(x, y)];
}

void
upro_substmat_set(struct upro_substmat *mat, upro_amino x, upro_amino y, double dist)
{
    mat->dists[UPRO_DISTMAT_INDEX(x, y)] = dist;
}

int
upro_substmat_load_many(struct upro_substmat *mat, size_t n, const char *path,
        enum upro_io_type iotype)
{
    int res;
    size_t i, j, k, rows, cols;

    struct upro_matrix matrix;
    res = upro_matrix_load_file(&matrix, path, iotype);
    if (res != UPRO_SUCCESS) {
        return res;
    }

    upro_matrix_dimensions(&matrix, &rows, &cols);
    if (rows * cols != n * UPRO_ALPHABET_SIZE * UPRO_ALPHABET_SIZE) {
        res = UPRO_FAILURE;
        goto error;
    }

    for (i = 0; i < n; i++) {
        for (j = 0; j < UPRO_ALPHABET_SIZE; j++) {
            for (k = 0; k < UPRO_ALPHABET_SIZE; k++) {
                /* treat `matrix` like a vector of length
                 *   n * UPRO_ALPHABET_SIZE * UPRO_ALPHABET_SIZE
                 * (this assumes upro_matrix uses a linear representation)
                 */
                size_t idx = (i * UPRO_ALPHABET_SIZE + j) * UPRO_ALPHABET_SIZE + k;
                upro_substmat_set(&mat[i], k, j, upro_matrix_get(&matrix, 0, idx));
            }
        }
    }
error:
    upro_matrix_destroy(&matrix);
    return res;
}

int
upro_substmat_load(struct upro_substmat *mat, const char *path,
        enum upro_io_type iotype)
{
    return upro_substmat_load_many(mat, 1, path, iotype);
}

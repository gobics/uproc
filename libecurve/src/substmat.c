#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ecurve/common.h"
#include "ecurve/alphabet.h"
#include "ecurve/io.h"
#include "ecurve/matrix.h"
#include "ecurve/substmat.h"

#define EC_DISTMAT_INDEX(x, y) ((x) << EC_AMINO_BITS | (y))

int
ec_substmat_init(struct ec_substmat *mat)
{
    *mat = (struct ec_substmat) { { 0.0 } };
    return EC_SUCCESS;
}

double
ec_substmat_get(const struct ec_substmat *mat, ec_amino x, ec_amino y)
{
    return mat->dists[EC_DISTMAT_INDEX(x, y)];
}

void
ec_substmat_set(struct ec_substmat *mat, ec_amino x, ec_amino y, double dist)
{
    mat->dists[EC_DISTMAT_INDEX(x, y)] = dist;
}

int
ec_substmat_load_many(struct ec_substmat *mat, size_t n, const char *path,
        enum ec_io_type iotype)
{
    int res;
    size_t i, j, k, rows, cols;

    struct ec_matrix matrix;
    res = ec_matrix_load_file(&matrix, path, iotype);
    if (res != EC_SUCCESS) {
        return res;
    }

    ec_matrix_dimensions(&matrix, &rows, &cols);
    if (rows * cols != n * EC_ALPHABET_SIZE * EC_ALPHABET_SIZE) {
        res = EC_FAILURE;
        goto error;
    }

    for (i = 0; i < n; i++) {
        for (j = 0; j < EC_ALPHABET_SIZE; j++) {
            for (k = 0; k < EC_ALPHABET_SIZE; k++) {
                /* treat `matrix` like a vector of length
                 *   n * EC_ALPHABET_SIZE * EC_ALPHABET_SIZE
                 * (this assumes ec_matrix uses a linear representation)
                 */
                size_t idx = (i * EC_ALPHABET_SIZE + j) * EC_ALPHABET_SIZE + k;
                ec_substmat_set(&mat[i], k, j, ec_matrix_get(&matrix, 0, idx));
            }
        }
    }
error:
    ec_matrix_destroy(&matrix);
    return res;
}

int
ec_substmat_load(struct ec_substmat *mat, const char *path,
        enum ec_io_type iotype)
{
    return ec_substmat_load_many(mat, 1, path, iotype);
}

/* Compute alignment distances between amino acid words
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/alphabet.h"
#include "uproc/io.h"
#include "uproc/matrix.h"
#include "uproc/substmat.h"

struct uproc_substmat_s
{
    /** Matrix containing distances */
    double dists[UPROC_SUFFIX_LEN][UPROC_ALPHABET_SIZE << UPROC_AMINO_BITS];
};

#define SUBSTMAT_INDEX(x, y) ((x) << UPROC_AMINO_BITS | (y))

uproc_substmat *uproc_substmat_create(void)
{
    struct uproc_substmat_s *mat = malloc(sizeof *mat);
    if (!mat) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    *mat = (struct uproc_substmat_s){{{0.0}}};
    return mat;
}

void uproc_substmat_destroy(uproc_substmat *mat)
{
    free(mat);
}

uproc_substmat *uproc_substmat_eye(void)
{
    uproc_substmat *mat = uproc_substmat_create();
    if (!mat) {
        return NULL;
    }
    for (unsigned pos = 0; pos < UPROC_SUFFIX_LEN; pos++) {
        for (uproc_amino a = 0; a < UPROC_ALPHABET_SIZE; a++) {
            uproc_substmat_set(mat, pos, a, a, 1.0);
        }
    }
    return mat;
}

double uproc_substmat_get(const uproc_substmat *mat, unsigned pos,
                          uproc_amino x, uproc_amino y)
{
    return mat->dists[pos][SUBSTMAT_INDEX(x, y)];
}

void uproc_substmat_set(uproc_substmat *mat, unsigned pos, uproc_amino x,
                        uproc_amino y, double dist)
{
    mat->dists[pos][SUBSTMAT_INDEX(x, y)] = dist;
}

void uproc_substmat_align_suffixes(const uproc_substmat *mat, uproc_suffix s1,
                                   uproc_suffix s2,
                                   double dist[static UPROC_SUFFIX_LEN])
{
    size_t i, idx;
    uproc_amino a1, a2;
    for (i = 0; i < UPROC_SUFFIX_LEN; i++) {
        a1 = s1 & UPROC_BITMASK(UPROC_AMINO_BITS);
        a2 = s2 & UPROC_BITMASK(UPROC_AMINO_BITS);
        s1 >>= UPROC_AMINO_BITS;
        s2 >>= UPROC_AMINO_BITS;
        idx = UPROC_SUFFIX_LEN - i - 1;
        dist[idx] = uproc_substmat_get(mat, idx, a1, a2);
    }
}

uproc_substmat *uproc_substmat_loadv(enum uproc_io_type iotype,
                                     const char *pathfmt, va_list ap)
{
    struct uproc_substmat_s *mat;
    unsigned long i, j, k, rows, cols, sz, required_sz;
    uproc_matrix *matrix;

    mat = uproc_substmat_create();
    if (!mat) {
        return NULL;
    }

    matrix = uproc_matrix_loadv(iotype, pathfmt, ap);
    if (!matrix) {
        uproc_substmat_destroy(mat);
        return NULL;
    }

    uproc_matrix_dimensions(matrix, &rows, &cols);
    sz = rows * cols;
    required_sz = UPROC_SUFFIX_LEN * UPROC_ALPHABET_SIZE * UPROC_ALPHABET_SIZE;
    if (sz != required_sz) {
        uproc_error_msg(UPROC_EINVAL,
                        "invalid substmat (%lu elements instead of %lu)", sz,
                        required_sz);
        goto error;
    }

    for (i = 0; i < UPROC_SUFFIX_LEN; i++) {
        for (j = 0; j < UPROC_ALPHABET_SIZE; j++) {
            for (k = 0; k < UPROC_ALPHABET_SIZE; k++) {
                /* treat `matrix` like a vector of length SUBSTMAT_REQUIRED
                 * (this assumes uproc_matrix uses a linear representation) */
                size_t idx;
                idx = (i * UPROC_ALPHABET_SIZE + j) * UPROC_ALPHABET_SIZE + k;
                uproc_substmat_set(mat, i, k, j,
                                   uproc_matrix_get(matrix, 0, idx));
            }
        }
    }
error:
    uproc_matrix_destroy(matrix);
    return mat;
}

uproc_substmat *uproc_substmat_load(enum uproc_io_type iotype,
                                    const char *pathfmt, ...)
{
    struct uproc_substmat_s *mat;
    va_list ap;
    va_start(ap, pathfmt);
    mat = uproc_substmat_loadv(iotype, pathfmt, ap);
    va_end(ap);
    return mat;
}

/* Two-dimensional `double` matrix
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
#include <stdio.h>
#include <string.h>
#include <limits.h>

#if HAVE_LAPACKE_H
#include <lapacke.h>
#endif

#include "uproc/matrix.h"
#include "uproc/error.h"
#include "uproc/common.h"
#include "uproc/io.h"

/** printf() format for matrix file header */
#define MATRIX_HEADER_PRI "[%lu, %lu]\n"

/** scanf() format for matrix file header */
#define MATRIX_HEADER_SCN "[%lu , %lu]"

/** Matrix */
struct uproc_matrix_s
{
    /** Number of rows */
    unsigned long rows;

    /** Number of columns */
    unsigned long cols;

    /** Values (as "flat" array) */
    double *values;
};

uproc_matrix *uproc_matrix_create(unsigned long rows, unsigned long cols,
                                  const double *values)
{
    struct uproc_matrix_s *matrix;

    /* unlikely, but nevertheless... */
    if (SIZE_MAX / cols <= rows) {
        uproc_error_msg(UPROC_EINVAL, "matrix too large");
        return NULL;
    }

    matrix = malloc(sizeof *matrix);
    if (!matrix) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    if (rows == 1) {
        rows = cols;
        cols = 1;
    }
    matrix->rows = rows;
    matrix->cols = cols;

    matrix->values = malloc(rows * cols * sizeof *matrix->values);
    if (!matrix->values) {
        free(matrix);
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    if (values) {
        memcpy(matrix->values, values, rows * cols * sizeof *matrix->values);
    } else {
        for (unsigned long i = 0; i < rows * cols; i++) {
            matrix->values[i] = 0.0;
        }
    }
    return matrix;
}

void uproc_matrix_destroy(uproc_matrix *matrix)
{
    if (!matrix) {
        return;
    }
    free(matrix->values);
    free(matrix);
}

uproc_matrix *uproc_matrix_copy(const uproc_matrix *m)
{
    return uproc_matrix_create(m->rows, m->cols, m->values);
}

void uproc_matrix_set(uproc_matrix *matrix, unsigned long row,
                      unsigned long col, double value)
{
    matrix->values[row * matrix->cols + col] = value;
}

double uproc_matrix_get(const uproc_matrix *matrix, unsigned long row,
                        unsigned long col)
{
    return matrix->values[row * matrix->cols + col];
}

void uproc_matrix_dimensions(const uproc_matrix *matrix, unsigned long *rows,
                             unsigned long *cols)
{
    *rows = matrix->rows;
    *cols = matrix->cols;
}

uproc_matrix *uproc_matrix_eye(unsigned long size, double factor)
{
    uproc_matrix *m = uproc_matrix_create(size, size, NULL);
    if (!m) {
        return NULL;
    }
    for (unsigned long i = 0; factor != 0.0 && i < size; i++) {
        uproc_matrix_set(m, i, i, factor);
    }
    return m;
}

void uproc_matrix_add_elem(uproc_matrix *matrix, unsigned long row,
                           unsigned long col, double val)
{
    matrix->values[row * matrix->cols + col] += val;
}

uproc_matrix *uproc_matrix_add(const uproc_matrix *a, const uproc_matrix *b)
{
    if (a->rows != b->rows || a->cols != b->cols) {
        uproc_error_msg(UPROC_EINVAL,
                        "dimension mismatch: [%lu, %lu] != [%lu, %lu]",
                        a->rows, a->cols, b->rows, b->cols);
        return NULL;
    }

    uproc_matrix *c = uproc_matrix_create(a->rows, a->cols, a->values);
    if (!c) {
        return NULL;
    }
    for (unsigned long row = 0; row < a->rows; row++) {
        for (unsigned long col = 0; col < a->cols; col++) {
            double va = uproc_matrix_get(a, row, col),
                   vb = uproc_matrix_get(b, row, col);
            uproc_matrix_set(c, row, col, va + vb);
        }
    }
    return c;
}

uproc_matrix *uproc_matrix_ldivide(const uproc_matrix *a, const uproc_matrix *b)
{
#if !HAVE_LAPACKE_DGESV
    uproc_error_msg(UPROC_ENOTSUP, "not supported");
    return NULL;
#else
    if (a->rows != a->cols) {
        uproc_error_msg(UPROC_EINVAL,
                        "invalid dimensions [%lu, %lu], square matrix required",
                        a->rows, a->cols);
        return NULL;
    }
    if (a->rows != b->rows) {
        uproc_error_msg(UPROC_EINVAL,
                        "dimension mismatch: [%lu, %lu] vs. [%lu, %lu]",
                        a->rows, a->cols, b->rows, b->cols);
        return NULL;
    }

    lapack_int n;
    if (sizeof (lapack_int) == sizeof(int)) {
        if (a->rows > INT_MAX) {
            uproc_error_msg(UPROC_EINVAL,
                            "matrix too big, %lu > %d", a->rows, INT_MAX);
            return NULL;
        }
        n = (int)a->rows;
    }
    if (sizeof (lapack_int) == sizeof(long)) {
        if (a->rows > LONG_MAX) {
            uproc_error_msg(UPROC_EINVAL,
                            "matrix too big, %lu > %ld", a->rows, LONG_MAX);
            return NULL;
        }
        n = (long)a->rows;
    }

    uproc_matrix *x = uproc_matrix_copy(b);
    if (!x) {
        return NULL;
    }
    lapack_int *ipiv = malloc(n * sizeof *ipiv);
    if (!ipiv) {
        uproc_error(UPROC_ENOMEM);
        goto error;
    }

    lapack_int info = LAPACKE_dgesv(LAPACK_ROW_MAJOR, n, 1, a->values, n, ipiv,
                                    x->values, 1);
    if (info != 0) {
        uproc_error_msg(UPROC_FAILURE,
                        "lapack returned error: %ld", (long)info);
        goto error;
    }
    free(ipiv);
    return x;

error:
    free(ipiv);
    uproc_matrix_destroy(x);
    return NULL;
#endif
}

uproc_matrix *uproc_matrix_loads(uproc_io_stream *stream)
{
    struct uproc_matrix_s *matrix;
    unsigned long i, k, rows, cols;
    double val;
    char buf[1024];

    if (!uproc_io_gets(buf, sizeof buf, stream) ||
        sscanf(buf, MATRIX_HEADER_SCN, &rows, &cols) != 2) {
        uproc_error_msg(UPROC_EINVAL, "invalid matrix header");
        return NULL;
    }

    matrix = uproc_matrix_create(rows, cols, NULL);
    if (!matrix) {
        return NULL;
    }

    for (i = 0; i < rows; i++) {
        for (k = 0; k < cols; k++) {
            if (!uproc_io_gets(buf, sizeof buf, stream) ||
                sscanf(buf, "%lf", &val) != 1) {
                uproc_matrix_destroy(matrix);
                uproc_error_msg(UPROC_EINVAL, "invalid value or EOF");
                return NULL;
            }
            uproc_matrix_set(matrix, i, k, val);
        }
    }
    return matrix;
}

uproc_matrix *uproc_matrix_loadv(enum uproc_io_type iotype, const char *pathfmt,
                                 va_list ap)
{
    struct uproc_matrix_s *matrix;
    uproc_io_stream *stream = uproc_io_openv("r", iotype, pathfmt, ap);
    if (!stream) {
        return NULL;
    }
    matrix = uproc_matrix_loads(stream);
    (void)uproc_io_close(stream);
    return matrix;
}

uproc_matrix *uproc_matrix_load(enum uproc_io_type iotype, const char *pathfmt,
                                ...)
{
    struct uproc_matrix_s *matrix;
    va_list ap;
    va_start(ap, pathfmt);
    matrix = uproc_matrix_loadv(iotype, pathfmt, ap);
    va_end(ap);
    return matrix;
}

int uproc_matrix_stores(const uproc_matrix *matrix, uproc_io_stream *stream)
{
    int res;
    unsigned long i, k, rows, cols;
    uproc_matrix_dimensions(matrix, &rows, &cols);
    res = uproc_io_printf(stream, MATRIX_HEADER_PRI, rows, cols);
    if (res < 0) {
        return -1;
    }
    for (i = 0; i < rows; i++) {
        for (k = 0; k < cols; k++) {
            res = uproc_io_printf(stream, "%lf\n",
                                  uproc_matrix_get(matrix, i, k));
            if (res < 0) {
                return -1;
            }
        }
    }
    return 0;
}

int uproc_matrix_storev(const uproc_matrix *matrix, enum uproc_io_type iotype,
                        const char *pathfmt, va_list ap)
{
    int res;
    uproc_io_stream *stream = uproc_io_openv("w", iotype, pathfmt, ap);
    if (!stream) {
        return -1;
    }
    res = uproc_matrix_stores(matrix, stream);
    uproc_io_close(stream);
    return res;
}

int uproc_matrix_store(const uproc_matrix *matrix, enum uproc_io_type iotype,
                       const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_matrix_storev(matrix, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}

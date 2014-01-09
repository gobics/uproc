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

#include "uproc/matrix.h"
#include "uproc/error.h"
#include "uproc/common.h"
#include "uproc/io.h"

/** Matrix */
struct uproc_matrix_s
{
    /** Number of rows */
    size_t rows;

    /** Number of columns */
    size_t cols;

    /** Values (as "flat" array) */
    double *values;
};

uproc_matrix *
uproc_matrix_create(size_t rows, size_t cols, const double *values)
{
    struct uproc_matrix_s *matrix = malloc(sizeof *matrix);
    if (!matrix) {
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
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    if (values) {
        memcpy(matrix->values, values, rows * cols * sizeof *matrix->values);
    }
    return matrix;
}

void
uproc_matrix_destroy(uproc_matrix *matrix)
{
    free(matrix->values);
    free(matrix);
}

void
uproc_matrix_set(uproc_matrix *matrix, size_t row, size_t col,
                 double value)
{
    matrix->values[row * matrix->cols + col] = value;
}

double
uproc_matrix_get(const uproc_matrix *matrix, size_t row, size_t col)
{
    return matrix->values[row * matrix->cols + col];
}

void
uproc_matrix_dimensions(const uproc_matrix *matrix, size_t *rows, size_t *cols)
{
    *rows = matrix->rows;
    *cols = matrix->cols;
}

uproc_matrix *
uproc_matrix_loads(uproc_io_stream *stream)
{
    struct uproc_matrix_s *matrix;
    size_t i, k, rows, cols;
    double val;
    char buf[1024];

    if (!uproc_io_gets(buf, sizeof buf, stream) ||
        sscanf(buf, UPROC_MATRIX_HEADER_SCN, &rows, &cols) != 2)
    {
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

uproc_matrix *
uproc_matrix_loadv(enum uproc_io_type iotype, const char *pathfmt, va_list ap)
{
    struct uproc_matrix_s *matrix;
    uproc_io_stream *stream = uproc_io_openv("r", iotype, pathfmt, ap);
    if (!stream) {
        return NULL;
    }
    matrix = uproc_matrix_loads(stream);
    (void) uproc_io_close(stream);
    return matrix;
}

uproc_matrix *
uproc_matrix_load(enum uproc_io_type iotype, const char *pathfmt, ...)
{
    struct uproc_matrix_s *matrix;
    va_list ap;
    va_start(ap, pathfmt);
    matrix = uproc_matrix_loadv(iotype, pathfmt, ap);
    va_end(ap);
    return matrix;
}

int
uproc_matrix_stores(const uproc_matrix *matrix, uproc_io_stream *stream)
{
    int res;
    size_t i, k, rows, cols;
    uproc_matrix_dimensions(matrix, &rows, &cols);
    res = uproc_io_printf(stream, UPROC_MATRIX_HEADER_PRI, rows, cols);
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

int
uproc_matrix_storev(const uproc_matrix *matrix, enum uproc_io_type iotype,
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

int
uproc_matrix_store(const uproc_matrix *matrix, enum uproc_io_type iotype,
                   const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = uproc_matrix_storev(matrix, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}

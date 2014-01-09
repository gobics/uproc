/** \file uproc/matrix.h
 *
 * Two-dimensional `double` matrix
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

#ifndef UPROC_THRESHOLDS_H
#define UPROC_THRESHOLDS_H


#include <stdlib.h>
#include <stdarg.h>
#include "uproc/io.h"

typedef struct uproc_matrix_s uproc_matrix;

/** printf() format for matrix file header */
#define UPROC_MATRIX_HEADER_PRI "[%zu, %zu]\n"

/** scanf() format for matrix file header */
#define UPROC_MATRIX_HEADER_SCN "[%zu , %zu]"

/** Initialize matrix
 *
 * If `values` is not a null pointer, it should point into an array of size
 * `rows * cols` containing a "flat" representation of the matrix, one row
 * followed by the other.
 *
 * \param matrix    matrix object
 * \param rows      number of rows
 * \param cols      number of columns
 * \param values    initial values
 *
 * \retval UPROC_SUCCESS   success
 * \retval UPROC_ENOMEM    memory allocation failed
 */
uproc_matrix *uproc_matrix_create(size_t rows, size_t cols,
                                  const double *values);

/** Free resources of a matrix */
void uproc_matrix_destroy(uproc_matrix *matrix);

/** Set the value of matrix[row, col] */
void uproc_matrix_set(uproc_matrix *matrix, size_t row, size_t col,
                      double value);

/** Get the value of matrix[row, col] */
double uproc_matrix_get(const uproc_matrix *matrix, size_t row, size_t col);

/** Obtain matrix dimensions
 *
 * \param matrix    matrix object
 * \param rows      _OUT_: number of rows
 * \param cols      _OUT_: number of columns
 */
void uproc_matrix_dimensions(const uproc_matrix *matrix, size_t *rows,
                             size_t *cols);

/** Load matrix from stream */
uproc_matrix *uproc_matrix_loads(uproc_io_stream *stream);

/** Load matrix from file */
uproc_matrix *uproc_matrix_load(enum uproc_io_type iotype, const char *pathfmt,
                                ...);

uproc_matrix *uproc_matrix_loadv(enum uproc_io_type iotype,
                                 const char *pathfmt, va_list ap);

/** Store matrix to stream */
int uproc_matrix_stores(const uproc_matrix *matrix, uproc_io_stream *stream);

/** Store matrix to file */
int uproc_matrix_store(const uproc_matrix *matrix, enum uproc_io_type iotype,
                       const char *pathfmt, ...);

int uproc_matrix_storev(const uproc_matrix *matrix, enum uproc_io_type iotype,
                        const char *pathfmt, va_list ap);
#endif

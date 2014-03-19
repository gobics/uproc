/* Copyright 2014 Peter Meinicke, Robin Martinjak
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

/** \file uproc/matrix.h
 *
 * Module: \ref grp_datastructs_matrix
 *
 * \weakgroup grp_datastructs
 * \{
 *
 * \weakgroup grp_datastructs_matrix
 * \{
 */

#ifndef UPROC_THRESHOLDS_H
#define UPROC_THRESHOLDS_H


#include <stdlib.h>
#include <stdarg.h>
#include "uproc/io.h"

/** \defgroup obj_matrix object uproc_matrix
 *
 * 2D matrix
 *
 * A very basic matrix implementation that is only useful to look up values
 * depending on the two dimensions and storing to or loading from a
 * plain text file. Neither bounds checking nor any algrebraic or arithmetic
 * operations are supported. Trying to retrieve or modify an element that is
 * beyond the bounds results in undefined behaviour.
 *
 * \{
 */

/** \struct uproc_matrix
 * \copybrief obj_matrix
 *
 * See \ref obj_matrix for details.
 */
typedef struct uproc_matrix_s uproc_matrix;


/** Create matrix object
 *
 * If \c values is not NULL, it should point into an array of size
 * <tt>rows * cols</tt> containing a "flat" representation of the matrix, one
 * row followed by the other, which will be copied into the new matrix object.
 * If \c values is NULL, the contents will be initialized to 0.0.
 *
 * \param rows      number of rows
 * \param cols      number of columns
 * \param values    initial values or NULL
 */
uproc_matrix *uproc_matrix_create(size_t rows, size_t cols,
                                  const double *values);


/** Destroy matrix object */
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


/** Load matrix from file
 *
 * \param iotype    IO type, see ::uproc_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 */
uproc_matrix *uproc_matrix_load(enum uproc_io_type iotype, const char *pathfmt,
                                ...);


/** Load matrix from file
 *
 * Like ::uproc_matrix_load, but with a \c va_list instead of a variable
 * number of arguments.
 */
uproc_matrix *uproc_matrix_loadv(enum uproc_io_type iotype,
                                 const char *pathfmt, va_list ap);


/** Store matrix to stream */
int uproc_matrix_stores(const uproc_matrix *matrix, uproc_io_stream *stream);


/** Store matrix to file */
int uproc_matrix_store(const uproc_matrix *matrix, enum uproc_io_type iotype,
                       const char *pathfmt, ...);


/** Store matrix to file
 *
 * Like ::uproc_matrix_store, but with a \c va_list instead of a variable
 * number of arguments.
 */
int uproc_matrix_storev(const uproc_matrix *matrix, enum uproc_io_type iotype,
                        const char *pathfmt, va_list ap);
/** \} */

/**
 * \}
 * \}
 */
#endif

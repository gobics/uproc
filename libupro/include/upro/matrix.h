#ifndef UPRO_THRESHOLDS_H
#define UPRO_THRESHOLDS_H

/** \file upro/matrix.h
 *
 * Two-dimensional `double` matrix
 */

#include <stdlib.h>
#include <stdarg.h>
#include "upro/io.h"

/** printf() format for matrix file header */
#define UPRO_MATRIX_HEADER_PRI "[%zu, %zu]\n"

/** scanf() format for matrix file header */
#define UPRO_MATRIX_HEADER_SCN "[%zu , %zu]"

/** Matrix struct definition */
struct upro_matrix {
    /** Number of rows */
    size_t rows;

    /** Number of columns */
    size_t cols;

    /** Values (as "flat" array) */
    double *values;
};

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
 * \retval UPRO_SUCCESS   success
 * \retval UPRO_ENOMEM    memory allocation failed
 */
int upro_matrix_init(struct upro_matrix *matrix, size_t rows, size_t cols,
                   const double *values);

/** Free resources of a matrix */
void upro_matrix_destroy(struct upro_matrix *matrix);

/** Set the value of matrix[row, col] */
void upro_matrix_set(struct upro_matrix *matrix, size_t row, size_t col,
                   double value);

/** Get the value of matrix[row, col] */
double upro_matrix_get(const struct upro_matrix *matrix, size_t row, size_t col);

/** Obtain matrix dimensions
 *
 * \param matrix    matrix object
 * \param rows      _OUT_: number of rows
 * \param cols      _OUT_: number of columns
 */
void upro_matrix_dimensions(const struct upro_matrix *matrix, size_t *rows,
        size_t *cols);

/** Load matrix from file */
int upro_matrix_load(struct upro_matrix *matrix, enum upro_io_type iotype,
        const char *pathfmt, ...);

int upro_matrix_loadv(struct upro_matrix *matrix, enum upro_io_type iotype,
        const char *pathfmt, va_list ap);

/** Store matrix to file */
int upro_matrix_store(const struct upro_matrix *matrix,
        enum upro_io_type iotype, const char *pathfmt, ...);

int upro_matrix_storev(const struct upro_matrix *matrix,
        enum upro_io_type iotype, const char *pathfmt, va_list ap);
#endif

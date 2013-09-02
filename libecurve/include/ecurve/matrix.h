#ifndef EC_THRESHOLDS_H
#define EC_THRESHOLDS_H

/** \file ecurve/matrix.h
 *
 * Two-dimensional `double` matrix
 */

#include <stdlib.h>

/** printf() format for matrix file header */
#define EC_MATRIX_HEADER_PRI "[%zu, %zu]\n"

/** scanf() format for matrix file header */
#define EC_MATRIX_HEADER_SCN "[%zu , %zu]"

/** Matrix struct definition */
struct ec_matrix {
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
 * \retval EC_SUCCESS   success
 * \retval EC_ENOMEM    memory allocation failed
 */
int ec_matrix_init(struct ec_matrix *matrix, size_t rows, size_t cols,
                   const double *values);

/** Free resources of a matrix */
void ec_matrix_destroy(struct ec_matrix *matrix);

/** Set the value of matrix[row, col] */
void ec_matrix_set(struct ec_matrix *matrix, size_t row, size_t col,
                   double value);

/** Get the value of matrix[row, col] */
double ec_matrix_get(const struct ec_matrix *matrix, size_t row, size_t col);

/** Obtain matrix dimensions
 *
 * \param matrix    matrix object
 * \param rows      _OUT_: number of rows
 * \param cols      _OUT_: number of columns
 */
void ec_matrix_dimensions(const struct ec_matrix *matrix, size_t *rows, size_t *cols);

/** Load matrix from file */
int ec_matrix_load_file(struct ec_matrix *matrix, const char *path);

/** Load matrix from stream */
int ec_matrix_load_stream(struct ec_matrix *matrix, FILE *stream);

/** Store matrix to file */
int ec_matrix_store_file(const struct ec_matrix *matrix, const char *path);

/** Store matrix to stream */
int ec_matrix_store_stream(const struct ec_matrix *matrix, FILE *stream);

#endif

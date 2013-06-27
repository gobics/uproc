#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "ecurve/matrix.h"
#include "ecurve/common.h"

int
ec_matrix_init(struct ec_matrix *matrix, size_t rows, size_t cols,
               const double *values)
{
    matrix->rows = rows;
    matrix->cols = cols;

    matrix->values = malloc(rows * cols * sizeof *matrix->values);
    if (!matrix->values) {
        return EC_FAILURE;
    }
    if (values) {
        memcpy(matrix->values, values, rows * cols * sizeof *matrix->values);
    }
    return EC_SUCCESS;
}

void
ec_matrix_destroy(struct ec_matrix *matrix)
{
    free(matrix->values);
}

void
ec_matrix_set(struct ec_matrix *matrix, size_t row, size_t col, double value)
{
    matrix->values[row * matrix->cols + col] = value;
}

double
ec_matrix_get(const struct ec_matrix *matrix, size_t row, size_t col)
{
    return matrix->values[row * matrix->cols + col];
}

void
ec_matrix_dimensions(const struct ec_matrix *matrix, size_t *rows, size_t *cols)
{
    *rows = matrix->rows;
    *cols = matrix->cols;
}

int
ec_matrix_load_file(struct ec_matrix *matrix, const char *path)
{
    int res;
    FILE *stream = fopen(path, "r");
    if (!stream) {
        return EC_FAILURE;
    }
    res = ec_matrix_load_stream(matrix, stream);
    fclose(stream);
    return res;
}

int
ec_matrix_load_stream(struct ec_matrix *matrix, FILE *stream)
{
    int res;
    size_t i, k, rows, cols;
    double val;

    if (fscanf(stream, EC_MATRIX_HEADER_SCN, &rows, &cols) != 2) {
        return EC_FAILURE;
    }

    res = ec_matrix_init(matrix, rows, cols, NULL);
    if (res != EC_SUCCESS) {
        return res;
    }

    for (i = 0; i < rows; i++) {
        for (k = 0; k < cols; k++) {
            if (fscanf(stream, "%lf", &val) != 1) {
                ec_matrix_destroy(matrix);
                return EC_FAILURE;
            }
            ec_matrix_set(matrix, i, k, val);
        }
    }
    return EC_SUCCESS;
}

int
ec_matrix_store_file(const struct ec_matrix *matrix, const char *path)
{
    int res;
    FILE *stream = fopen(path, "w");
    if (!stream) {
        return EC_FAILURE;
    }
    res = ec_matrix_store_stream(matrix, stream);
    fclose(stream);
    return res;
}

int
ec_matrix_store_stream(const struct ec_matrix *matrix, FILE *stream)
{
    int res;
    size_t i, k, rows, cols;
    ec_matrix_dimensions(matrix, &rows, &cols);
    res = fprintf(stream, EC_MATRIX_HEADER_PRI, rows, cols);
    if (res < 0) {
        return EC_FAILURE;
    }
    for (i = 0; i < rows; i++) {
        for (k = 0; k < cols; k++) {
            res = fprintf(stream, "%lf\n", ec_matrix_get(matrix, i, k));
            if (res < 0) {
                return EC_FAILURE;
            }
        }
    }
    return EC_SUCCESS;
}

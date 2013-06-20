#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "ecurve/matrix.h"
#include "ecurve/common.h"

int
ec_matrix_init(ec_matrix *matrix, size_t rows, size_t cols,
               const double *values)
{
    struct ec_matrix_s *m = matrix;

    m->rows = rows;
    m->cols = cols;

    m->values = malloc(rows * cols * sizeof *m->values);
    if (!m->values) {
        return EC_FAILURE;
    }
    if (values) {
        memcpy(m->values, values, rows * cols * sizeof *m->values);
    }
    return EC_SUCCESS;
}

void
ec_matrix_destroy(ec_matrix *matrix)
{
    struct ec_matrix_s *m = matrix;
    free(m->values);
}

void
ec_matrix_set(ec_matrix *matrix, size_t row, size_t col, double value)
{
    struct ec_matrix_s *m = matrix;
    m->values[row * m->cols + col] = value;
}

double
ec_matrix_get(const ec_matrix *matrix, size_t row, size_t col)
{
    const struct ec_matrix_s *m = matrix;
    return m->values[row * m->cols + col];
}

void
ec_matrix_dimensions(const ec_matrix *matrix, size_t *rows, size_t *cols)
{
    const struct ec_matrix_s *m = matrix;
    *rows = m->rows;
    *cols = m->cols;
}

int
ec_matrix_load_file(ec_matrix *matrix, const char *path)
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
ec_matrix_load_stream(ec_matrix *matrix, FILE *stream)
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
ec_matrix_store_file(const ec_matrix *matrix, const char *path)
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
ec_matrix_store_stream(const ec_matrix *matrix, FILE *stream)
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "ecurve/matrix.h"
#include "ecurve/common.h"
#include "ecurve/storage.h"
#include "ecurve/io.h"

int
ec_matrix_init(struct ec_matrix *matrix, size_t rows, size_t cols,
               const double *values)
{
    if (rows == 1) {
        rows = cols;
        cols = 1;
    }
    matrix->rows = rows;
    matrix->cols = cols;

    matrix->values = malloc(rows * cols * sizeof *matrix->values);
    if (!matrix->values) {
        return EC_ENOMEM;
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
ec_matrix_load_file(struct ec_matrix *matrix, const char *path,
        enum ec_io_type iotype)
{
    int res;
    ec_io_stream *stream = ec_io_open(path, "r", iotype);
    if (!stream) {
        return EC_FAILURE;
    }
    res = ec_matrix_load_stream(matrix, stream);
    (void) ec_io_close(stream);
    return res;
}

int
ec_matrix_load_stream(struct ec_matrix *matrix, ec_io_stream *stream)
{
    int res;
    size_t i, k, rows, cols;
    double val;
    char buf[1024];

    if (!ec_io_gets(buf, sizeof buf, stream) ||
        sscanf(buf, EC_MATRIX_HEADER_SCN, &rows, &cols) != 2)
    {
        return EC_EINVAL;
    }

    res = ec_matrix_init(matrix, rows, cols, NULL);
    if (EC_ISERROR(res)) {
        return res;
    }

    for (i = 0; i < rows; i++) {
        for (k = 0; k < cols; k++) {
            if (!ec_io_gets(buf, sizeof buf, stream) ||
                sscanf(buf, "%lf", &val) != 1) {
                ec_matrix_destroy(matrix);
                return EC_EINVAL;
            }
            ec_matrix_set(matrix, i, k, val);
        }
    }
    return EC_SUCCESS;
}

int
ec_matrix_store_file(const struct ec_matrix *matrix, const char *path,
        enum ec_io_type iotype)
{
    int res;
    char *mode = "w";
    ec_io_stream *stream = ec_io_open(path, mode, iotype);
    if (!stream) {
        return EC_FAILURE;
    }
    res = ec_matrix_store_stream(matrix, stream);
    ec_io_close(stream);
    return res;
}

int
ec_matrix_store_stream(const struct ec_matrix *matrix, ec_io_stream *stream)
{
    int res;
    size_t i, k, rows, cols;
    ec_matrix_dimensions(matrix, &rows, &cols);
    res = ec_io_printf(stream, EC_MATRIX_HEADER_PRI, rows, cols);
    if (res < 0) {
        return EC_FAILURE;
    }
    for (i = 0; i < rows; i++) {
        for (k = 0; k < cols; k++) {
            res = ec_io_printf(stream, "%lf\n", ec_matrix_get(matrix, i, k));
            if (res < 0) {
                return EC_FAILURE;
            }
        }
    }
    return EC_SUCCESS;
}

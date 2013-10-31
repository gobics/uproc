#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "upro/matrix.h"
#include "upro/common.h"
#include "upro/storage.h"
#include "upro/io.h"

int
upro_matrix_init(struct upro_matrix *matrix, size_t rows, size_t cols,
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
        return UPRO_ENOMEM;
    }
    if (values) {
        memcpy(matrix->values, values, rows * cols * sizeof *matrix->values);
    }
    return UPRO_SUCCESS;
}

void
upro_matrix_destroy(struct upro_matrix *matrix)
{
    free(matrix->values);
}

void
upro_matrix_set(struct upro_matrix *matrix, size_t row, size_t col, double value)
{
    matrix->values[row * matrix->cols + col] = value;
}

double
upro_matrix_get(const struct upro_matrix *matrix, size_t row, size_t col)
{
    return matrix->values[row * matrix->cols + col];
}

void
upro_matrix_dimensions(const struct upro_matrix *matrix, size_t *rows, size_t *cols)
{
    *rows = matrix->rows;
    *cols = matrix->cols;
}

int
upro_matrix_load_file(struct upro_matrix *matrix, const char *path,
        enum upro_io_type iotype)
{
    int res;
    upro_io_stream *stream = upro_io_open(path, "r", iotype);
    if (!stream) {
        return UPRO_FAILURE;
    }
    res = upro_matrix_load_stream(matrix, stream);
    (void) upro_io_close(stream);
    return res;
}

int
upro_matrix_load_stream(struct upro_matrix *matrix, upro_io_stream *stream)
{
    int res;
    size_t i, k, rows, cols;
    double val;
    char buf[1024];

    if (!upro_io_gets(buf, sizeof buf, stream) ||
        sscanf(buf, UPRO_MATRIX_HEADER_SCN, &rows, &cols) != 2)
    {
        return UPRO_EINVAL;
    }

    res = upro_matrix_init(matrix, rows, cols, NULL);
    if (UPRO_ISERROR(res)) {
        return res;
    }

    for (i = 0; i < rows; i++) {
        for (k = 0; k < cols; k++) {
            if (!upro_io_gets(buf, sizeof buf, stream) ||
                sscanf(buf, "%lf", &val) != 1) {
                upro_matrix_destroy(matrix);
                return UPRO_EINVAL;
            }
            upro_matrix_set(matrix, i, k, val);
        }
    }
    return UPRO_SUCCESS;
}

int
upro_matrix_store_file(const struct upro_matrix *matrix, const char *path,
        enum upro_io_type iotype)
{
    int res;
    char *mode = "w";
    upro_io_stream *stream = upro_io_open(path, mode, iotype);
    if (!stream) {
        return UPRO_FAILURE;
    }
    res = upro_matrix_store_stream(matrix, stream);
    upro_io_close(stream);
    return res;
}

int
upro_matrix_store_stream(const struct upro_matrix *matrix, upro_io_stream *stream)
{
    int res;
    size_t i, k, rows, cols;
    upro_matrix_dimensions(matrix, &rows, &cols);
    res = upro_io_printf(stream, UPRO_MATRIX_HEADER_PRI, rows, cols);
    if (res < 0) {
        return UPRO_FAILURE;
    }
    for (i = 0; i < rows; i++) {
        for (k = 0; k < cols; k++) {
            res = upro_io_printf(stream, "%lf\n", upro_matrix_get(matrix, i, k));
            if (res < 0) {
                return UPRO_FAILURE;
            }
        }
    }
    return UPRO_SUCCESS;
}

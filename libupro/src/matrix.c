#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "upro/matrix.h"
#include "upro/error.h"
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
        return upro_error(UPRO_ENOMEM);
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

static int
matrix_load(struct upro_matrix *matrix, upro_io_stream *stream)
{
    int res;
    size_t i, k, rows, cols;
    double val;
    char buf[1024];

    if (!upro_io_gets(buf, sizeof buf, stream) ||
        sscanf(buf, UPRO_MATRIX_HEADER_SCN, &rows, &cols) != 2)
    {
        return upro_error_msg(UPRO_EINVAL, "invalid matrix header");
    }

    res = upro_matrix_init(matrix, rows, cols, NULL);
    if (res) {
        return res;
    }

    for (i = 0; i < rows; i++) {
        for (k = 0; k < cols; k++) {
            if (!upro_io_gets(buf, sizeof buf, stream) ||
                sscanf(buf, "%lf", &val) != 1) {
                upro_matrix_destroy(matrix);
                return upro_error_msg(UPRO_EINVAL, "invalid value or EOF");
            }
            upro_matrix_set(matrix, i, k, val);
        }
    }
    return UPRO_SUCCESS;
}

int
upro_matrix_loadv(struct upro_matrix *matrix, enum upro_io_type iotype,
        const char *pathfmt, va_list ap)
{
    int res;
    upro_io_stream *stream = upro_io_openv("r", iotype, pathfmt, ap);
    if (!stream) {
        return UPRO_FAILURE;
    }
    res = matrix_load(matrix, stream);
    (void) upro_io_close(stream);
    return res;
}

int
upro_matrix_load(struct upro_matrix *matrix, enum upro_io_type iotype,
        const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = upro_matrix_loadv(matrix, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}

static int
matrix_store(const struct upro_matrix *matrix, upro_io_stream *stream)
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

int
upro_matrix_storev(const struct upro_matrix *matrix, enum upro_io_type iotype,
        const char *pathfmt, va_list ap)
{
    int res;
    upro_io_stream *stream = upro_io_openv("w", iotype, pathfmt, ap);
    if (!stream) {
        return UPRO_FAILURE;
    }
    res = matrix_store(matrix, stream);
    upro_io_close(stream);
    return res;
}

int
upro_matrix_store(const struct upro_matrix *matrix, enum upro_io_type iotype,
        const char *pathfmt, ...)
{
    int res;
    va_list ap;
    va_start(ap, pathfmt);
    res = upro_matrix_storev(matrix, iotype, pathfmt, ap);
    va_end(ap);
    return res;
}

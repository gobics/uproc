#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "ecurve/matrix.h"
#include "ecurve/common.h"
#include "ecurve/storage.h"
#include "ecurve/gz.h"

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
ec_matrix_load_file(struct ec_matrix *matrix, const char *path)
{
    int res;
    gzFile stream = gzopen(path, "r");
    if (!stream) {
        return EC_ESYSCALL;
    }
    (void) gzbuffer(stream, EC_GZ_BUFSZ);
    res = ec_matrix_load_stream(matrix, stream);
    gzclose(stream);
    return res;
}

int
ec_matrix_load_stream(struct ec_matrix *matrix, gzFile stream)
{
    int res;
    size_t i, k, rows, cols;
    double val;
    char buf[1024];

    if (!gzgets(stream, buf, sizeof buf) ||
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
            if (!gzgets(stream, buf, sizeof buf) ||
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
                     int flags)
{
    int res;
    char *mode = "w";
#if HAVE_ZLIB
    if (!(flags & EC_STORAGE_GZIP)) {
        mode = "wT";
    }
#else
    (void) flags;
#endif
    gzFile stream = gzopen(path, mode);
    if (!stream) {
        return EC_ESYSCALL;
    }
    (void) gzbuffer(stream, EC_GZ_BUFSZ);
    res = ec_matrix_store_stream(matrix, stream);
    gzclose(stream);
    return res;
}

int
ec_matrix_store_stream(const struct ec_matrix *matrix, gzFile stream)
{
    int res;
    size_t i, k, rows, cols;
    ec_matrix_dimensions(matrix, &rows, &cols);
    res = gzprintf(stream, EC_MATRIX_HEADER_PRI, rows, cols);
    if (res < 0) {
        return EC_FAILURE;
    }
    for (i = 0; i < rows; i++) {
        for (k = 0; k < cols; k++) {
            res = gzprintf(stream, "%lf\n", ec_matrix_get(matrix, i, k));
            if (res < 0) {
                return EC_FAILURE;
            }
        }
    }
    return EC_SUCCESS;
}

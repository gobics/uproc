#include <math.h>

#include "test.h"
#include "upro.h"

#define TMPFILE "t/tmpfile"

void setup(void)
{
}

void teardown(void)
{
}

int init(void)
{
    struct upro_matrix m;
    double v[10 * 10] = {
        [1] = 0.5,
        [11] = 1.0, 1.5,
        [25] = 2.0,
    };

    DESC("initialization with values");

    upro_matrix_init(&m, 10, 10, v);

#define T(i, r, c) assert_double_eq(v[i], upro_matrix_get(&m, r, c), "correct value");
    T(1, 0, 1);
    T(2, 0, 2);
    T(11, 1, 1);
    T(12, 1, 2);
    T(25, 2, 5);
    T(26, 2, 6);
#undef T
    upro_matrix_destroy(&m);
    return SUCCESS;
}

int vector(void)
{
    int i;
    struct upro_matrix v1, v2;
    double values[100] = { [4] = 3.4, [11] = 4.5, [27] = 3.14, [98] = 42.0 };
    DESC("vector-like matrices");
    upro_matrix_init(&v1, 1, 100, values);
    upro_matrix_init(&v2, 100, 1, values);
    for (i = 0; i < 100; i++) {
        assert_double_eq(upro_matrix_get(&v1, 0, i), upro_matrix_get(&v1, i, 0), "v1 symmetric");
        assert_double_eq(upro_matrix_get(&v2, 0, i), upro_matrix_get(&v2, i, 0), "v2 symmetric");
        assert_double_eq(upro_matrix_get(&v1, i, 0), upro_matrix_get(&v2, i, 0), "v1 eq v2 (by row)");
        assert_double_eq(upro_matrix_get(&v1, 0, i), upro_matrix_get(&v2, 0, i), "v1 eq v2 (by col)");
    }
    upro_matrix_destroy(&v1);
    upro_matrix_destroy(&v2);
    return SUCCESS;
}

int storage(void)
{
    struct upro_matrix m;
    double v[10 * 10] = {
        [1] = 0.5,
        [11] = 1.0, 1.5,
        [25] = 2.0,
    };

    DESC("storing/loading matrix");

    upro_matrix_init(&m, 10, 10, v);

    assert_int_eq(upro_matrix_store_file(&m, TMPFILE, UPRO_IO_GZIP),
                  UPRO_SUCCESS, "storing matrix succeeded");
    upro_matrix_destroy(&m);
    assert_int_eq(upro_matrix_load_file(&m, TMPFILE, UPRO_IO_GZIP),
                  UPRO_SUCCESS, "loading matrix succeeded");
#define T(i, r, c) assert_double_eq(v[i], upro_matrix_get(&m, r, c), "correct value");
    T(1, 0, 1);
    T(2, 0, 2);
    T(11, 1, 1);
    T(12, 1, 2);
    T(25, 2, 5);
    T(26, 2, 6);
#undef T
    upro_matrix_destroy(&m);
    return UPRO_SUCCESS;
}


TESTS_INIT(init, vector, storage);

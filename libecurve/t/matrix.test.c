#include <math.h>

#include "test.h"
#include "ecurve.h"

#define TMPFILE "t/tmpfile"

void setup(void)
{
}

void teardown(void)
{
}

int init(void)
{
    struct ec_matrix m;
    double v[10 * 10] = {
        [1] = 0.5,
        [11] = 1.0, 1.5,
        [25] = 2.0,
    };

    DESC("initialization with values");

    ec_matrix_init(&m, 10, 10, v);

#define T(i, r, c) assert_double_eq(v[i], ec_matrix_get(&m, r, c), "correct value");
    T(1, 0, 1);
    T(2, 0, 2);
    T(11, 1, 1);
    T(12, 1, 2);
    T(25, 2, 5);
    T(26, 2, 6);
#undef T
    ec_matrix_destroy(&m);
    return SUCCESS;
}

int vector(void)
{
    int i;
    struct ec_matrix v1, v2;
    double values[100] = { [4] = 3.4, [11] = 4.5, [27] = 3.14, [98] = 42.0 };
    DESC("vector-like matrices");
    ec_matrix_init(&v1, 1, 100, values);
    ec_matrix_init(&v2, 100, 1, values);
    for (i = 0; i < 100; i++) {
        assert_double_eq(ec_matrix_get(&v1, 0, i), ec_matrix_get(&v1, i, 0), "v1 symmetric");
        assert_double_eq(ec_matrix_get(&v2, 0, i), ec_matrix_get(&v2, i, 0), "v2 symmetric");
        assert_double_eq(ec_matrix_get(&v1, i, 0), ec_matrix_get(&v2, i, 0), "v1 eq v2 (by row)");
        assert_double_eq(ec_matrix_get(&v1, 0, i), ec_matrix_get(&v2, 0, i), "v1 eq v2 (by col)");
    }
    ec_matrix_destroy(&v1);
    ec_matrix_destroy(&v2);
    return SUCCESS;
}

int storage(void)
{
    struct ec_matrix m;
    double v[10 * 10] = {
        [1] = 0.5,
        [11] = 1.0, 1.5,
        [25] = 2.0,
    };

    DESC("storing/loading matrix");

    ec_matrix_init(&m, 10, 10, v);

    assert_int_eq(ec_matrix_store_file(&m, TMPFILE, EC_IO_GZIP),
                  EC_SUCCESS, "storing matrix succeeded");
    ec_matrix_destroy(&m);
    assert_int_eq(ec_matrix_load_file(&m, TMPFILE, EC_IO_GZIP),
                  EC_SUCCESS, "loading matrix succeeded");
#define T(i, r, c) assert_double_eq(v[i], ec_matrix_get(&m, r, c), "correct value");
    T(1, 0, 1);
    T(2, 0, 2);
    T(11, 1, 1);
    T(12, 1, 2);
    T(25, 2, 5);
    T(26, 2, 6);
#undef T
    ec_matrix_destroy(&m);
    return EC_SUCCESS;
}


TESTS_INIT(init, vector, storage);

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
    ec_matrix m;
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

int storage(void)
{
    ec_matrix m;
    double v[10 * 10] = {
        [1] = 0.5,
        [11] = 1.0, 1.5,
        [25] = 2.0,
    };

    DESC("storing/loading matrix");

    ec_matrix_init(&m, 10, 10, v);

    assert_int_eq(ec_matrix_store_file(&m, TMPFILE),
                  EC_SUCCESS, "storing matrix succeeded");
    ec_matrix_destroy(&m);
    assert_int_eq(ec_matrix_load_file(&m, TMPFILE),
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


TESTS_INIT(init, storage);

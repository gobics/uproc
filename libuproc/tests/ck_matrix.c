#include <errno.h>
#include <check.h>
#include "uproc.h"

#define assert_dbl_eq(a, b, name) do { \
    double _a = a, _b = b; \
    ck_assert_msg(fabs(_a - _b) <= UPROC_EPSILON, \
        "Assertion '%s' in test '%s' failed: %s == %g, %s == %g", \
        "|(" #a ")-(" #b ")| <= eps", (name), #a, _a, #b, _b, UPROC_EPSILON); \
    } while (0)

uproc_matrix *m;

START_TEST(test_init)
{
    double data[] = {0.0, 0.1, 0.2, 1.0, 1.1, 1.2, 2.0, 2.1,
                     2.2, 3.0, 3.1, 3.2, 4.0, 4.1, 4.2};

    uproc_matrix *mat = uproc_matrix_create(5, 3, data);
    ck_assert_ptr_ne(mat, NULL);

    ck_assert(uproc_matrix_get(mat, 0, 0) == 0.0);
    ck_assert(uproc_matrix_get(mat, 0, 2) == 0.2);
    ck_assert(uproc_matrix_get(mat, 1, 0) == 1.0);
    ck_assert(uproc_matrix_get(mat, 1, 2) == 1.2);
    ck_assert(uproc_matrix_get(mat, 3, 1) == 3.1);
    ck_assert(uproc_matrix_get(mat, 4, 0) == 4.0);
    ck_assert(uproc_matrix_get(mat, 4, 2) == 4.2);
    uproc_matrix_destroy(mat);
}
END_TEST

START_TEST(test_init_vector)
{
    size_t rows, cols;
    uproc_matrix *mat;
    mat = uproc_matrix_create(42, 1, NULL);
    uproc_matrix_dimensions(mat, &rows, &cols);
    ck_assert_uint_eq(rows, 42);
    ck_assert_uint_eq(cols, 1);
    uproc_matrix_destroy(mat);

    mat = uproc_matrix_create(1, 42, NULL);
    uproc_matrix_dimensions(mat, &rows, &cols);
    ck_assert_uint_eq(rows, 42);
    ck_assert_uint_eq(cols, 1);
    uproc_matrix_destroy(mat);
}
END_TEST

START_TEST(test_store_load)
{
    int res;
    double data[] = {0.0, 0.1, 0.2, 0.3, 1.0, 1.1, 1.2, 1.3};

    uproc_matrix *mat = uproc_matrix_create(2, 4, data);

    res = uproc_matrix_store(mat, UPROC_IO_GZIP, TMPDATADIR "test_matrix.tmp");
    ck_assert_msg(res == 0, "storing failed");
    uproc_matrix_destroy(mat);

    mat = uproc_matrix_load(UPROC_IO_GZIP, TMPDATADIR "test_matrix.tmp");
    ck_assert_ptr_ne(mat, NULL);

    size_t rows, cols;
    uproc_matrix_dimensions(mat, &rows, &cols);
    ck_assert_uint_eq(rows, 2);
    ck_assert_uint_eq(cols, 4);

    ck_assert(uproc_matrix_get(mat, 0, 0) == 0.0);
    ck_assert(uproc_matrix_get(mat, 0, 2) == 0.2);
    ck_assert(uproc_matrix_get(mat, 0, 3) == 0.3);
    ck_assert(uproc_matrix_get(mat, 1, 0) == 1.0);
    ck_assert(uproc_matrix_get(mat, 1, 2) == 1.2);
    ck_assert(uproc_matrix_get(mat, 1, 3) == 1.3);
    uproc_matrix_destroy(mat);
}
END_TEST

START_TEST(test_load_invalid)
{
    uproc_matrix *mat;
    mat = uproc_matrix_load(UPROC_IO_GZIP, DATADIR "no_such_file");
    ck_assert_ptr_eq(mat, NULL);
    ck_assert_int_eq(uproc_errno, UPROC_ERRNO);
    ck_assert_int_eq(errno, ENOENT);

    mat = uproc_matrix_load(UPROC_IO_GZIP, DATADIR "missing_header.matrix");
    ck_assert_ptr_eq(mat, NULL);
    ck_assert_int_eq(uproc_errno, UPROC_EINVAL);

    mat = uproc_matrix_load(UPROC_IO_GZIP, DATADIR "invalid_header.matrix");
    ck_assert_ptr_eq(mat, NULL);
    ck_assert_int_eq(uproc_errno, UPROC_EINVAL);
}
END_TEST

START_TEST(test_add)
{
    struct {
        unsigned long rows, cols;
        double *a, *b, *want;
    } tests[] = {
        {
            1, 1,
            (double[]) { 1 },
            (double[]) { 2 },
            (double[]) { 3 },
        },
        {
            2, 1,
            (double[]) { 1, 40 },
            (double[]) { 2, 2 },
            (double[]) { 3, 42 },
        },
    };
    for (size_t i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        unsigned long rows = tests[i].rows, cols = tests[i].cols;

        uproc_matrix *a = uproc_matrix_create(rows, cols, tests[i].a),
                     *b = uproc_matrix_create(rows, cols, tests[i].b),
                     *want = uproc_matrix_create(rows, cols, tests[i].want),
                     *got = uproc_matrix_add(a, b);
        for (unsigned long row = 0; i < rows; i++) {
            for (unsigned long col = 0; i < cols; i++) {
                assert_dbl_eq(uproc_matrix_get(got, row, col),
                              uproc_matrix_get(want, row, col),
                              "test_add");
            }
        }
        uproc_matrix_destroy(a);
        uproc_matrix_destroy(b);
        uproc_matrix_destroy(got);
        uproc_matrix_destroy(want);
    }
}
END_TEST

START_TEST(test_ldivide)
{
    double va[] = {
        17,  24,   1,   8,  15,
        23,   5,   7,  14,  16,
        4,   6,  13,  20,  22,
        10,  12,  19,  21,   3,
        11,  18,  25,   2,   9,
    }, vb[] = { 65, 65, 65, 65, 65 };

    uproc_matrix *a = uproc_matrix_create(5, 5, va),
                 *b = uproc_matrix_create(5, 1, vb),
                 *x = uproc_matrix_ldivide(a, b);

    if (!x) {
        uproc_perror("");
        ck_assert(0);
    }

    for (unsigned long i = 0; i < 5; i++) {
        double val = uproc_matrix_get(x, i, 0);
        assert_dbl_eq(val, 1.0, "test_ldivide");
    }
    uproc_matrix_destroy(a);
    uproc_matrix_destroy(b);
    uproc_matrix_destroy(x);
}
END_TEST

int main(void)
{
    Suite *s = suite_create("matrix");

    TCase *tc = tcase_create("matrix");
    tcase_add_test(tc, test_init);
    tcase_add_test(tc, test_init_vector);
    tcase_add_test(tc, test_store_load);
    tcase_add_test(tc, test_load_invalid);
    tcase_add_test(tc, test_add);
    tcase_add_test(tc, test_ldivide);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

#include <errno.h>
#include <check.h>
#include "uproc.h"

uproc_matrix *m;

START_TEST(test_init)
{
    uproc_matrix *mat;
    double data[] = {0.0, 0.1, 0.2, 1.0, 1.1, 1.2, 2.0, 2.1,
                     2.2, 3.0, 3.1, 3.2, 4.0, 4.1, 4.2};

    mat = uproc_matrix_create(5, 3, data);
    ck_assert_ptr_ne(mat, NULL);

    ck_assert(uproc_matrix_get(mat, 0, 0) == 0.0);
    ck_assert(uproc_matrix_get(mat, 0, 2) == 0.2);
    ck_assert(uproc_matrix_get(mat, 1, 0) == 1.0);
    ck_assert(uproc_matrix_get(mat, 1, 2) == 1.2);
    ck_assert(uproc_matrix_get(mat, 3, 1) == 3.1);
    ck_assert(uproc_matrix_get(mat, 4, 0) == 4.0);
    ck_assert(uproc_matrix_get(mat, 4, 2) == 4.2);
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
    uproc_matrix *mat;
    size_t rows, cols;
    double data[] = {0.0, 0.1, 0.2, 0.3, 1.0, 1.1, 1.2, 1.3};

    mat = uproc_matrix_create(2, 4, data);

    res = uproc_matrix_store(mat, UPROC_IO_GZIP, TMPDATADIR "test.matrix");
    ck_assert_msg(res == 0, "storing failed");
    uproc_matrix_destroy(mat);

    mat = uproc_matrix_load(UPROC_IO_GZIP, TMPDATADIR "test.matrix");
    ck_assert_ptr_ne(mat, NULL);

    uproc_matrix_dimensions(mat, &rows, &cols);
    ck_assert_uint_eq(rows, 2);
    ck_assert_uint_eq(cols, 4);

    ck_assert(uproc_matrix_get(mat, 0, 0) == 0.0);
    ck_assert(uproc_matrix_get(mat, 0, 2) == 0.2);
    ck_assert(uproc_matrix_get(mat, 0, 3) == 0.3);
    ck_assert(uproc_matrix_get(mat, 1, 0) == 1.0);
    ck_assert(uproc_matrix_get(mat, 1, 2) == 1.2);
    ck_assert(uproc_matrix_get(mat, 1, 3) == 1.3);
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

int main(void)
{
    Suite *s = suite_create("matrix");

    TCase *tc = tcase_create("matrix");
    tcase_add_test(tc, test_init);
    tcase_add_test(tc, test_init_vector);
    tcase_add_test(tc, test_store_load);
    tcase_add_test(tc, test_load_invalid);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

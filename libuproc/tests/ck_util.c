#include "check_workaround.h"
#include "../util.h"

#include <stdlib.h>
#include <stdint.h>

START_TEST(test_array_reverse)
{
    intmax_t a[] = {5, 4, 3, 2, 1, 0};
    ARRAY_REVERSE(a);
    for (int i = 0; i < sizeof a / sizeof *a; i++) {
        ck_assert_int_eq(a[i], i);
    }
}
END_TEST

int main(void)
{
    Suite *s = suite_create("util.h");

    TCase *tc = tcase_create("utility functions");
    tcase_add_test(tc, test_array_reverse);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

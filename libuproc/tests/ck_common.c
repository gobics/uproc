#include <check.h>
#include "uproc.h"

START_TEST(test_defs)
{
    ck_assert_msg(
        UPROC_PREFIX_MAX == pow(UPROC_ALPHABET_SIZE, UPROC_PREFIX_LEN) - 1,
        "UPROC_PREFIX_MAX is alphabet size raised to prefix length");
    ck_assert_msg(
        ~(uproc_prefix)0 > UPROC_PREFIX_MAX,
        "uproc_prefix large enough (can store UPROC_PREFIX_MAX + 1)");
    ck_assert_msg(
        sizeof (uproc_suffix) * CHAR_BIT >= (UPROC_AMINO_BITS * UPROC_SUFFIX_LEN),
        "uproc_suffix large enough");
}
END_TEST

int main(void)
{
    Suite *s = suite_create("common definitions");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_defs);
    suite_add_tcase(s, tc_core);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

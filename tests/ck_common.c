#include <check.h>
#include "upro.h"

START_TEST(test_defs)
{
    ck_assert_msg(
        UPRO_PREFIX_MAX == pow(UPRO_ALPHABET_SIZE, UPRO_PREFIX_LEN) - 1,
        "UPRO_PREFIX_MAX is alphabet size raised to prefix length");
    ck_assert_msg(
        ~(upro_prefix)0 > UPRO_PREFIX_MAX,
        "upro_prefix large enough (can store UPRO_PREFIX_MAX + 1)");
    ck_assert_msg(
        sizeof (upro_suffix) * CHAR_BIT >= (UPRO_AMINO_BITS * UPRO_SUFFIX_LEN),
        "upro_suffix large enough");
}
END_TEST

Suite *
common_suite(void)
{
    Suite *s = suite_create("common definitions");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_defs);
    suite_add_tcase(s, tc_core);
    return s;
}

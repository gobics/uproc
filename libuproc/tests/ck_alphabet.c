#include "check_workaround.h"
#include "uproc.h"

uproc_alphabet *a;

START_TEST(test_init_too_short)
{
    uproc_errno = UPROC_SUCCESS;
    ck_assert_msg(uproc_alphabet_create("ABC") == NULL,
                  "failure if string too short");
    ck_assert_int_eq(uproc_errno, UPROC_EINVAL);
}
END_TEST

START_TEST(test_init_too_long)
{
    uproc_errno = UPROC_SUCCESS;
    ck_assert_msg(uproc_alphabet_create("ABCDEFGHIJKLMNOPQRSTUVW") == NULL,
                  "failure if string too long");
    ck_assert_int_eq(uproc_errno, UPROC_EINVAL);
}
END_TEST

START_TEST(test_init_duplicates)
{
    uproc_errno = UPROC_SUCCESS;
    ck_assert_msg(uproc_alphabet_create("AACDEFGHIJKLMNOPQRST") == NULL,
                  "failure if string contains duplicates");
    ck_assert_int_eq(uproc_errno, UPROC_EINVAL);

    uproc_errno = UPROC_SUCCESS;
    ck_assert_msg(uproc_alphabet_create("ABCDEFGHIJKKMNOPQRST") == NULL,
                  "failure if string contains duplicates");
    ck_assert_int_eq(uproc_errno, UPROC_EINVAL);
}
END_TEST

START_TEST(test_init_invalid_chars)
{
    uproc_errno = UPROC_SUCCESS;
    ck_assert_msg(uproc_alphabet_create("ABCDE GHIJKLMNOPQRST") == NULL,
                  "failure if string contains space");
    ck_assert_int_eq(uproc_errno, UPROC_EINVAL);

    uproc_errno = UPROC_SUCCESS;
    ck_assert_msg(uproc_alphabet_create("ABCDE1GHIJKLMNOPQRST") == NULL,
                  "failure if string contains number");
    ck_assert_int_eq(uproc_errno, UPROC_EINVAL);

    uproc_errno = UPROC_SUCCESS;
    ck_assert_msg(uproc_alphabet_create("ABCDE*GHIJKLMNOPQRST") == NULL,
                  "failure if string contains asterisk");
    ck_assert_int_eq(uproc_errno, UPROC_EINVAL);
}
END_TEST

START_TEST(test_init_valid)
{
    uproc_alphabet *a;
    ck_assert_msg((a = uproc_alphabet_create("AGSTPKRQEDNHYWFMLIVC")) != NULL,
                  "success if string is valid");
    uproc_alphabet_destroy(a);
}
END_TEST

void setup_trans(void)
{
    a = uproc_alphabet_create("AGSTPKRQEDNHYWFMLIVC");
}

void teardown_trans(void)
{
    uproc_alphabet_destroy(a);
}

START_TEST(test_translate_c2a)
{
    ck_assert_int_eq(uproc_alphabet_char_to_amino(a, '!'), -1);
    ck_assert_int_eq(uproc_alphabet_char_to_amino(a, 'B'), -1);
    ck_assert_int_eq(uproc_alphabet_char_to_amino(a, 'A'), 0);
    ck_assert_int_eq(uproc_alphabet_char_to_amino(a, 'C'), 19);
    ck_assert_int_eq(uproc_alphabet_char_to_amino(a, 'D'), 9);
    ck_assert_int_eq(uproc_alphabet_char_to_amino(a, 'T'), 3);
}
END_TEST

START_TEST(test_translate_a2c)
{
    ck_assert_int_eq(uproc_alphabet_amino_to_char(a, 0), 'A');
    ck_assert_int_eq(uproc_alphabet_amino_to_char(a, 4), 'P');
    ck_assert_int_eq(uproc_alphabet_amino_to_char(a, 17), 'I');
    ck_assert_int_eq(uproc_alphabet_amino_to_char(a, 19), 'C');
    ck_assert_int_eq(uproc_alphabet_amino_to_char(a, 20), -1);
}
END_TEST

int main(void)
{
    Suite *s = suite_create("alphabet");

    TCase *tc_init = tcase_create("Initialization");
    tcase_add_test(tc_init, test_init_too_short);
    tcase_add_test(tc_init, test_init_too_long);
    tcase_add_test(tc_init, test_init_duplicates);
    tcase_add_test(tc_init, test_init_invalid_chars);
    tcase_add_test(tc_init, test_init_valid);
    suite_add_tcase(s, tc_init);

    TCase *tc_trans = tcase_create("Translation");
    tcase_add_checked_fixture(tc_trans, setup_trans, teardown_trans);
    tcase_add_test(tc_trans, test_translate_c2a);
    tcase_add_test(tc_trans, test_translate_a2c);
    suite_add_tcase(s, tc_trans);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

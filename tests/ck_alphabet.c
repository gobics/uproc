#include <check.h>
#include "upro.h"

START_TEST(test_init)
{
    struct upro_alphabet a;
    upro_errno = UPRO_SUCCESS;
    fprintf(stderr, "%d\n", upro_alphabet_init(&a, "ABC"));
    ck_assert_msg(upro_alphabet_init(&a, "ABC") < 0,
                  "failure if string too short");
    ck_assert_int_eq(upro_errno, UPRO_EINVAL);

    upro_errno = UPRO_SUCCESS;
    ck_assert_msg(upro_alphabet_init(&a, "ABCDEFGHIJKLMNOPQRSTUVW") < 0,
                  "failure if string too long");
    ck_assert_int_eq(upro_errno, UPRO_EINVAL);


    upro_errno = UPRO_SUCCESS;
    ck_assert_msg(upro_alphabet_init(&a, "AACDEFGHIJKLMNOPQRST") < 0,
                  "failure if string contains duplicates");
    ck_assert_int_eq(upro_errno, UPRO_EINVAL);

    upro_errno = UPRO_SUCCESS;
    ck_assert_msg(upro_alphabet_init(&a, "ABCDEFGHIJKKMNOPQRST") < 0,
                  "failure if string contains duplicates");
    ck_assert_int_eq(upro_errno, UPRO_EINVAL);


    upro_errno = UPRO_SUCCESS;
    ck_assert_msg(upro_alphabet_init(&a, "ABCDE GHIJKLMNOPQRST") < 0,
                  "failure if string contains space");
    ck_assert_int_eq(upro_errno, UPRO_EINVAL);

    upro_errno = UPRO_SUCCESS;
    ck_assert_msg(upro_alphabet_init(&a, "ABCDE1GHIJKLMNOPQRST") < 0,
                  "failure if string contains number");
    ck_assert_int_eq(upro_errno, UPRO_EINVAL);

    upro_errno = UPRO_SUCCESS;
    ck_assert_msg(upro_alphabet_init(&a, "ABCDE*GHIJKLMNOPQRST") < 0,
                  "failure if string contains asterisk");
    ck_assert_int_eq(upro_errno, UPRO_EINVAL);
}
END_TEST

Suite *
alphabet_suite(void)
{
    Suite *s = suite_create("alphabet");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core, test_init);
    suite_add_tcase(s, tc_core);
    return s;
}

#include "test.h"
#include "upro.h"

struct upro_alphabet a;

void setup(void)
{}
void teardown(void)
{}

int init_tooshort(void)
{
    DESC("init with too short alphabet string");
    upro_errno = UPRO_SUCCESS;
    assert_int_lt(upro_alphabet_init(&a, "ABC"), 0,
                  "failure if string too short");
    assert_int_eq(upro_errno, UPRO_EINVAL, "upro_errno set");
    return SUCCESS;
}

int init_toolong(void)
{
    DESC("init with too long alphabet string");
    upro_errno = UPRO_SUCCESS;
    assert_int_lt(upro_alphabet_init(&a, "ABCDEFGHIJKLMNOPQRSTUVW"), 0,
                  "failure if string too long");
    assert_int_eq(upro_errno, UPRO_EINVAL, "upro_errno set");
    return SUCCESS;
}

int init_dup(void)
{
    DESC("init with duplicates");
    upro_errno = UPRO_SUCCESS;
    assert_int_lt(upro_alphabet_init(&a, "AACDEFGHIJKLMNOPQRST"), 0,
                  "failure if string contains duplicates");
    assert_int_eq(upro_errno, UPRO_EINVAL, "upro_errno set");

    upro_errno = UPRO_SUCCESS;
    assert_int_lt(upro_alphabet_init(&a, "ABCDEFGHIJKKMNOPQRST"), 0,
                  "failure if string contains duplicates");
    assert_int_eq(upro_errno, UPRO_EINVAL, "upro_errno set");
    return SUCCESS;
}

int init_nonalpha(void)
{
    DESC("init with non-alphabetic characters");
    upro_errno = UPRO_SUCCESS;
    assert_int_lt(upro_alphabet_init(&a, "ABCDE GHIJKLMNOPQRST"), 0,
                  "failure if string contains space");
    assert_int_eq(upro_errno, UPRO_EINVAL, "upro_errno set");

    upro_errno = UPRO_SUCCESS;
    assert_int_lt(upro_alphabet_init(&a, "ABCDE1GHIJKLMNOPQRST"), 0,
                  "failure if string contains number");
    assert_int_eq(upro_errno, UPRO_EINVAL, "upro_errno set");

    upro_errno = UPRO_SUCCESS;
    assert_int_lt(upro_alphabet_init(&a, "ABCDE*GHIJKLMNOPQRST"), 0,
                  "failure if string contains asterisk");
    assert_int_eq(upro_errno, UPRO_EINVAL, "upro_errno set");
    return SUCCESS;
}

int init_correct(void)
{
    DESC("init with correct length alphabet string");
    upro_errno = UPRO_SUCCESS;
    assert_int_eq(upro_alphabet_init(&a, "ABCDEFGHIJKLMNOPQRST"), 0,
                  "success if string has correct length");
    assert_int_eq(upro_errno, UPRO_SUCCESS, "upro_errno not set");
    return SUCCESS;
}

TESTS_INIT(init_tooshort, init_toolong, init_dup, init_nonalpha, init_correct);

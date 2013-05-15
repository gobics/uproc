#include "test.h"
#include "ecurve.h"

ec_alphabet a;

void setup(void)
{}
void teardown(void)
{}

int init_tooshort(void)
{
    DESC("init with too short alphabet string");
    assert_int_ne(ec_alphabet_init(&a, "ABC"), EC_SUCCESS,
                  "failure if string too short");
    return SUCCESS;
}

int init_toolong(void)
{
    DESC("init with too long alphabet string");
    assert_int_ne(ec_alphabet_init(&a, "ABCDEFGHIJKLMNOPQRSTUVW"), EC_SUCCESS,
                  "failure if string too long");
    return SUCCESS;
}

int init_dup(void)
{
    DESC("init with duplicates");
    assert_int_ne(ec_alphabet_init(&a, "AACDEFGHIJKLMNOPQRST"), EC_SUCCESS,
                  "failure if string contains duplicates");
    assert_int_ne(ec_alphabet_init(&a, "ABCDEFGHIJKKMNOPQRST"), EC_SUCCESS,
                  "failure if string contains duplicates");
    return SUCCESS;
}

int init_nonalpha(void)
{
    DESC("init with non-alphabetic characters");
    assert_int_ne(ec_alphabet_init(&a, "ABCDE GHIJKLMNOPQRST"), EC_SUCCESS,
                  "failure if string contains space");
    assert_int_ne(ec_alphabet_init(&a, "ABCDE1GHIJKLMNOPQRST"), EC_SUCCESS,
                  "failure if string contains number");
    assert_int_ne(ec_alphabet_init(&a, "ABCDE*GHIJKLMNOPQRST"), EC_SUCCESS,
                  "failure if string contains asterisk");
    return SUCCESS;
}

int init_correct(void)
{
    DESC("init with correct length alphabet string");
    assert_int_eq(ec_alphabet_init(&a, "ABCDEFGHIJKLMNOPQRST"), EC_SUCCESS,
                  "success if string has correct length");
    return SUCCESS;
}

int init_default(void)
{
    DESC("init with default alphabet");
    assert_int_eq(ec_alphabet_init(&a, EC_ALPHABET_ALPHA_DEFAULT), EC_SUCCESS,
                  "success with default alphabet");
    return SUCCESS;
}

TESTS_INIT(init_tooshort, init_toolong, init_dup, init_nonalpha, init_correct, init_default);

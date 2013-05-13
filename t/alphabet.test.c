#include "test.h"
#include "ecurve.h"

void setup(void)
{}
void teardown(void)
{}

int init_tooshort(void)
{
    ec_alphabet a;
    DESC("init with too short alphabet string");
    assert_int_ne(ec_alphabet_init(&a, "ABC"), EC_SUCCESS,
                  "failure if string too short");
    return SUCCESS;
}

int init_toolong(void)
{
    ec_alphabet a;
    DESC("init with too long alphabet string");
    assert_int_ne(ec_alphabet_init(&a, "ABCDEFGHIJKLMNOPQRSTUVW"), EC_SUCCESS,
                  "failure if string too long");
    return SUCCESS;
}

int init_correct(void)
{
    ec_alphabet a;
    DESC("init with correct length alphabet string");
    assert_int_eq(ec_alphabet_init(&a, "ABCDEFGHIJKLMNOPQRST"), EC_SUCCESS,
                  "success if string has correct length");
    return SUCCESS;
}

int init_default(void)
{
    ec_alphabet a;
    DESC("init with default alphabet");
    assert_int_eq(ec_alphabet_init(&a, EC_ALPHABET_ALPHA_DEFAULT), EC_SUCCESS,
                  "success with default alphabet");
    return SUCCESS;
}

TESTS_INIT(init_tooshort, init_toolong, init_correct, init_default);

#include "test.h"
#include "ecurve.h"

struct ec_word word, control;
ec_alphabet alpha;

void setup(void)
{
    ec_alphabet_init(&alpha, EC_ALPHABET_ALPHA_DEFAULT);
}

void teardown(void)
{
}

int to_string(void)
{
    int res;
    char *old = "NERDGEEKPETERPETER",
         new[EC_WORD_LEN + 1];

    DESC("ec_word <-> string");

    res = ec_word_from_string(&word, old, &alpha);
    assert_int_eq(res, EC_SUCCESS, "string is a valid sequence");

    assert_int_eq(ec_word_to_string(new, &word, &alpha), EC_SUCCESS, "success");
    assert_str_eq(old, new, "conversion to word and back yields same string");

    return SUCCESS;
}

int append(void)
{
    int res;
    int i = 4;
    char *old =     "AAAAAAAAAAAAAAAAAA",
         *expect =  "AANERDNERDNERDNERD",
         new[EC_WORD_LEN + 1];

    DESC("appending amino acids to word");

    res = ec_word_from_string(&word, old, &alpha);
    assert_int_eq(res, EC_SUCCESS, "string is a valid sequence");

    while (i--) {
        ec_word_append(&word, ec_alphabet_char_to_amino(&alpha, 'N'));
        ec_word_append(&word, ec_alphabet_char_to_amino(&alpha, 'E'));
        ec_word_append(&word, ec_alphabet_char_to_amino(&alpha, 'R'));
        ec_word_append(&word, ec_alphabet_char_to_amino(&alpha, 'D'));
    }

    ec_word_to_string(new, &word, &alpha);
    assert_str_eq(expect, new,
                  "prepending yields expected string");

    return SUCCESS;
}

int prepend(void)
{
    int i = 4;
    char *old =     "AAAAAAAAAAAAAAAAAA",
         *expect =  "NERDNERDNERDNERDAA",
         new[EC_WORD_LEN + 1];

    DESC("prepending amino acids to word");

    ec_word_from_string(&word, old, &alpha);

    while (i--) {
        ec_word_prepend(&word, ec_alphabet_char_to_amino(&alpha, 'D'));
        ec_word_prepend(&word, ec_alphabet_char_to_amino(&alpha, 'R'));
        ec_word_prepend(&word, ec_alphabet_char_to_amino(&alpha, 'E'));
        ec_word_prepend(&word, ec_alphabet_char_to_amino(&alpha, 'N'));
    }

    ec_word_to_string(new, &word, &alpha);
    assert_str_eq(expect, new,
                  "prepending yields expected string");

    return SUCCESS;
}

TESTS_INIT(to_string, append, prepend);

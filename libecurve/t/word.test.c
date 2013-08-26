#include "test.h"
#include "ecurve.h"

struct ec_word word, control;
struct ec_alphabet alpha;

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

int startswith(void)
{
    ec_amino a = ec_alphabet_char_to_amino(&alpha, 'R');
    DESC("testing whether a word starts with a certain amino acid");

    ec_word_from_string(&word, "RRRRRRRRRRRRRRRRRR", &alpha);
    assert_int_eq(ec_word_startswith(&word, a), 1, "word starts with R");

    ec_word_append(&word, ec_alphabet_char_to_amino(&alpha, 'T'));
    assert_int_eq(ec_word_startswith(&word, a), 1,
            "after appending T, word still starts with R");

    a = ec_alphabet_char_to_amino(&alpha, 'V');
    ec_word_prepend(&word, a);
    assert_int_eq(ec_word_startswith(&word, a), 1,
            "after prepending V, word starts with V");

    a = ec_alphabet_char_to_amino(&alpha, 'S');
    ec_word_prepend(&word, a);
    assert_int_eq(ec_word_startswith(&word, a), 1,
            "after prepending S, word starts with S");

    return SUCCESS;
}

int iter(void)
{
    int res;
    struct ec_worditer iter;
    size_t index;
    struct ec_word fwd = EC_WORD_INITIALIZER, rev = EC_WORD_INITIALIZER;
    char seq[] = "RAAAAAAAAAAAAAAAAAGC!VVVVVVVVVVVVVVVVVVSD!!!", new[20];

    DESC("iterating a sequence");

    ec_worditer_init(&iter, seq, &alpha);

#define TEST(IDX, FWD, REV) do {                                            \
    res = ec_worditer_next(&iter, &index, &fwd, &rev);                      \
    assert_int_eq(res, EC_ITER_YIELD, "iterator yielded something");        \
    assert_uint_eq(index, IDX, "index correct");                            \
    ec_word_to_string(new, &fwd, &alpha);                                   \
    assert_str_eq(new, FWD, "fwd word correct");                            \
    ec_word_to_string(new, &rev, &alpha);                                   \
    assert_str_eq(new, REV, "rev word correct");                            \
} while (0)

    TEST(0,  "RAAAAAAAAAAAAAAAAA", "AAAAAAAAAAAAAAAAAR");
    TEST(1,  "AAAAAAAAAAAAAAAAAG", "GAAAAAAAAAAAAAAAAA");
    TEST(2,  "AAAAAAAAAAAAAAAAGC", "CGAAAAAAAAAAAAAAAA");
    TEST(21, "VVVVVVVVVVVVVVVVVV", "VVVVVVVVVVVVVVVVVV");
    TEST(22, "VVVVVVVVVVVVVVVVVS", "SVVVVVVVVVVVVVVVVV");
    TEST(23, "VVVVVVVVVVVVVVVVSD", "DSVVVVVVVVVVVVVVVV");

    res = ec_worditer_next(&iter, &index, &fwd, &rev);
    assert_int_eq(res, EC_ITER_STOP, "iterator exhausted");

    return SUCCESS;
#undef TEST
}

TESTS_INIT(to_string, append, prepend, startswith, iter);

#include "test.h"
#include "upro.h"

struct upro_word word, control;
struct upro_alphabet alpha;

void setup(void)
{
    upro_alphabet_init(&alpha, ALPHABET);
}

void teardown(void)
{
}

int to_string(void)
{
    int res;
    char *old = "NERDGEEKPETERPETER",
         new[UPRO_WORD_LEN + 1];

    DESC("upro_word <-> string");

    res = upro_word_from_string(&word, old, &alpha);
    assert_int_eq(res, UPRO_SUCCESS, "string is a valid sequence");

    assert_int_eq(upro_word_to_string(new, &word, &alpha), UPRO_SUCCESS, "success");
    assert_str_eq(old, new, "conversion to word and back yields same string");

    return SUCCESS;
}

int append(void)
{
    int res;
    int i = 4;
    char *old =     "AAAAAAAAAAAAAAAAAA",
         *expect =  "AANERDNERDNERDNERD",
         new[UPRO_WORD_LEN + 1];

    DESC("appending amino acids to word");

    res = upro_word_from_string(&word, old, &alpha);
    assert_int_eq(res, UPRO_SUCCESS, "string is a valid sequence");

    while (i--) {
        upro_word_append(&word, upro_alphabet_char_to_amino(&alpha, 'N'));
        upro_word_append(&word, upro_alphabet_char_to_amino(&alpha, 'E'));
        upro_word_append(&word, upro_alphabet_char_to_amino(&alpha, 'R'));
        upro_word_append(&word, upro_alphabet_char_to_amino(&alpha, 'D'));
    }

    upro_word_to_string(new, &word, &alpha);
    assert_str_eq(expect, new,
                  "prepending yields expected string");

    return SUCCESS;
}

int prepend(void)
{
    int i = 4;
    char *old =     "AAAAAAAAAAAAAAAAAA",
         *expect =  "NERDNERDNERDNERDAA",
         new[UPRO_WORD_LEN + 1];

    DESC("prepending amino acids to word");

    upro_word_from_string(&word, old, &alpha);

    while (i--) {
        upro_word_prepend(&word, upro_alphabet_char_to_amino(&alpha, 'D'));
        upro_word_prepend(&word, upro_alphabet_char_to_amino(&alpha, 'R'));
        upro_word_prepend(&word, upro_alphabet_char_to_amino(&alpha, 'E'));
        upro_word_prepend(&word, upro_alphabet_char_to_amino(&alpha, 'N'));
    }

    upro_word_to_string(new, &word, &alpha);
    assert_str_eq(expect, new,
                  "prepending yields expected string");

    return SUCCESS;
}

int startswith(void)
{
    upro_amino a = upro_alphabet_char_to_amino(&alpha, 'R');
    DESC("testing whether a word starts with a certain amino acid");

    upro_word_from_string(&word, "RRRRRRRRRRRRRRRRRR", &alpha);
    assert_int_eq(upro_word_startswith(&word, a), 1, "word starts with R");

    upro_word_append(&word, upro_alphabet_char_to_amino(&alpha, 'T'));
    assert_int_eq(upro_word_startswith(&word, a), 1,
            "after appending T, word still starts with R");

    a = upro_alphabet_char_to_amino(&alpha, 'V');
    upro_word_prepend(&word, a);
    assert_int_eq(upro_word_startswith(&word, a), 1,
            "after prepending V, word starts with V");

    a = upro_alphabet_char_to_amino(&alpha, 'S');
    upro_word_prepend(&word, a);
    assert_int_eq(upro_word_startswith(&word, a), 1,
            "after prepending S, word starts with S");

    return SUCCESS;
}

int iter(void)
{
    int res;
    struct upro_worditer iter;
    size_t index;
    struct upro_word fwd = UPRO_WORD_INITIALIZER, rev = UPRO_WORD_INITIALIZER;
    char seq[] = "RAAAAAAAAAAAAAAAAAGC!VVVVVVVVVVVVVVVVVVSD!!!", new[20];

    DESC("iterating a sequence");

    upro_worditer_init(&iter, seq, &alpha);

#define TEST(IDX, FWD, REV) do {                                            \
    res = upro_worditer_next(&iter, &index, &fwd, &rev);                      \
    assert_int_eq(res, UPRO_ITER_YIELD, "iterator yielded something");        \
    assert_uint_eq(index, IDX, "index correct");                            \
    upro_word_to_string(new, &fwd, &alpha);                                   \
    assert_str_eq(new, FWD, "fwd word correct");                            \
    upro_word_to_string(new, &rev, &alpha);                                   \
    assert_str_eq(new, REV, "rev word correct");                            \
} while (0)

    TEST(0,  "RAAAAAAAAAAAAAAAAA", "AAAAAAAAAAAAAAAAAR");
    TEST(1,  "AAAAAAAAAAAAAAAAAG", "GAAAAAAAAAAAAAAAAA");
    TEST(2,  "AAAAAAAAAAAAAAAAGC", "CGAAAAAAAAAAAAAAAA");
    TEST(21, "VVVVVVVVVVVVVVVVVV", "VVVVVVVVVVVVVVVVVV");
    TEST(22, "VVVVVVVVVVVVVVVVVS", "SVVVVVVVVVVVVVVVVV");
    TEST(23, "VVVVVVVVVVVVVVVVSD", "DSVVVVVVVVVVVVVVVV");

    res = upro_worditer_next(&iter, &index, &fwd, &rev);
    assert_int_eq(res, UPRO_SUCCESS, "iterator exhausted");

    return SUCCESS;
#undef TEST
}

TESTS_INIT(to_string, append, prepend, startswith, iter);

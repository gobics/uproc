#include <string.h>
#include <ctype.h>

#include "check_workaround.h"
#include "uproc.h"
#include "../codon_tables.h"

uproc_alphabet *alpha;

void setup(void)
{
    alpha = uproc_alphabet_create("AGSTPKRQEDNHYWFMLIVC");
    ck_assert_ptr_ne(alpha, NULL);
}

void teardown(void)
{
    uproc_alphabet_destroy(alpha);
}

START_TEST(test_from_to_string)
{
    int res;
    struct uproc_word word;
    char old[] = "SPKQPETATKRQEDYWMG", new[UPROC_WORD_LEN + 1];

    ck_assert_uint_eq(sizeof old, sizeof new);

    res = uproc_word_from_string(&word, old, alpha);
    ck_assert_int_eq(res, 0);

    res = uproc_word_to_string(new, &word, alpha);
    ck_assert_int_eq(res, 0);

    ck_assert_str_eq(old, new);
}
END_TEST

START_TEST(test_cmp)
{
    struct uproc_word w1, w2;

    uproc_word_from_string(&w1, "AAAAAAAAAAAAAAAAAA", alpha);

    uproc_word_from_string(&w2, "AAAAAAAAAAAAAAAAAA", alpha);
    ck_assert_int_eq(uproc_word_cmp(&w1, &w2), 0);

    uproc_word_from_string(&w2, "AAAAAAAAAAAAAAAAAG", alpha);
    ck_assert_int_eq(uproc_word_cmp(&w1, &w2), -1);
    ck_assert_int_eq(uproc_word_cmp(&w2, &w1), 1);

    uproc_word_from_string(&w2, "AGSTPPPPPPPPPPPPPP", alpha);
    ck_assert_int_eq(uproc_word_cmp(&w1, &w2), -1);
    ck_assert_int_eq(uproc_word_cmp(&w2, &w1), 1);

    uproc_word_from_string(&w1, "AGSTPPPPPPPPPTPPPP", alpha);
    ck_assert_int_eq(uproc_word_cmp(&w1, &w2), -1);
    ck_assert_int_eq(uproc_word_cmp(&w2, &w1), 1);

    uproc_word_from_string(&w1, "AGSTPPPPPPPPKTPPPP", alpha);
    ck_assert_int_eq(uproc_word_cmp(&w1, &w2), 1);
    ck_assert_int_eq(uproc_word_cmp(&w2, &w1), -1);

    uproc_word_from_string(&w2, "AGSTPPPPPPPPRTPPPP", alpha);
    ck_assert_int_eq(uproc_word_cmp(&w1, &w2), -1);
    ck_assert_int_eq(uproc_word_cmp(&w2, &w1), 1);

    uproc_word_from_string(&w1, "AGSTPPPPPPPPRTPPPP", alpha);
    ck_assert_int_eq(uproc_word_cmp(&w1, &w2), 0);
}
END_TEST

START_TEST(test_append)
{
    int res;
    struct uproc_word word;
    int i = 4;
    char *old = "AAAAAAAAAAAAAAAAAA", *expect = "AANERDNERDNERDNERD",
         new[UPROC_WORD_LEN + 1];

    res = uproc_word_from_string(&word, old, alpha);
    ck_assert_int_eq(res, 0);

    while (i--) {
        uproc_word_append(&word, uproc_alphabet_char_to_amino(alpha, 'N'));
        uproc_word_append(&word, uproc_alphabet_char_to_amino(alpha, 'E'));
        uproc_word_append(&word, uproc_alphabet_char_to_amino(alpha, 'R'));
        uproc_word_append(&word, uproc_alphabet_char_to_amino(alpha, 'D'));
    }

    uproc_word_to_string(new, &word, alpha);
    ck_assert_str_eq(expect, new);
}
END_TEST

START_TEST(test_prepend)
{
    int res;
    struct uproc_word word;
    int i = 4;
    char *old = "AAAAAAAAAAAAAAAAAA", *expect = "NERDNERDNERDNERDAA",
         new[UPROC_WORD_LEN + 1];

    res = uproc_word_from_string(&word, old, alpha);
    ck_assert_int_eq(res, 0);

    while (i--) {
        uproc_word_prepend(&word, uproc_alphabet_char_to_amino(alpha, 'D'));
        uproc_word_prepend(&word, uproc_alphabet_char_to_amino(alpha, 'R'));
        uproc_word_prepend(&word, uproc_alphabet_char_to_amino(alpha, 'E'));
        uproc_word_prepend(&word, uproc_alphabet_char_to_amino(alpha, 'N'));
    }

    uproc_word_to_string(new, &word, alpha);
    ck_assert_str_eq(expect, new);
}
END_TEST

START_TEST(test_startswith)
{
    struct uproc_word word;

    uproc_word_from_string(&word, "AAAAAAAAAAAAAAAAAAAA", alpha);
    ck_assert(
        uproc_word_startswith(&word, uproc_alphabet_char_to_amino(alpha, 'A')));

    uproc_word_prepend(&word, 15);
    ck_assert(uproc_word_startswith(&word, 15));

    uproc_word_prepend(&word, 10);
    ck_assert(uproc_word_startswith(&word, 10));

    uproc_word_append(&word, 0);
    ck_assert(uproc_word_startswith(&word, 15));
}
END_TEST

START_TEST(test_worditer)
{
    int res;
    uproc_worditer *iter;
    size_t index;
    struct uproc_word fwd, rev;
    char seq[] = "RAAAAAAAAAAAAAAAAAGC!VVVVVVVVVVVVVVVVVVSD!!!",
         new[UPROC_WORD_LEN + 1];

    iter = uproc_worditer_create(seq, alpha);

#define TEST(IDX, FWD, REV)                                  \
    do {                                                     \
        res = uproc_worditer_next(iter, &index, &fwd, &rev); \
        ck_assert_int_eq(res, 0);                            \
        ck_assert_uint_eq(index, IDX);                       \
        uproc_word_to_string(new, &fwd, alpha);              \
        ck_assert_str_eq(new, FWD);                          \
        uproc_word_to_string(new, &rev, alpha);              \
        ck_assert_str_eq(new, REV);                          \
    } while (0)

    TEST(0, "RAAAAAAAAAAAAAAAAA", "AAAAAAAAAAAAAAAAAR");
    TEST(1, "AAAAAAAAAAAAAAAAAG", "GAAAAAAAAAAAAAAAAA");
    TEST(2, "AAAAAAAAAAAAAAAAGC", "CGAAAAAAAAAAAAAAAA");
    TEST(21, "VVVVVVVVVVVVVVVVVV", "VVVVVVVVVVVVVVVVVV");
    TEST(22, "VVVVVVVVVVVVVVVVVS", "SVVVVVVVVVVVVVVVVV");
    TEST(23, "VVVVVVVVVVVVVVVVSD", "DSVVVVVVVVVVVVVVVV");

    res = uproc_worditer_next(iter, &index, &fwd, &rev);
    uproc_worditer_destroy(iter);
    ck_assert_int_eq(res, 1);
#undef TEST
}
END_TEST

int main(void)
{
    (void)codon_is_stop;
    (void)codon_to_char;

    Suite *s = suite_create("word");

    TCase *tc = tcase_create("");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_from_to_string);
    tcase_add_test(tc, test_cmp);
    tcase_add_test(tc, test_append);
    tcase_add_test(tc, test_prepend);
    tcase_add_test(tc, test_startswith);
    tcase_add_test(tc, test_worditer);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

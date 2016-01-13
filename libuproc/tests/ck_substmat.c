#include <string.h>
#include <ctype.h>

#include <check.h>
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

START_TEST(test_align_suffixes)
{
    int res;
    struct uproc_word word;
    char s1[] = "AAAAAAAAAAAAAAAAAA",
         s2[] = "PPPPPPAAAAAAAAATAA";
    struct uproc_word w1, w2;

    res = uproc_word_from_string(&w1, s1, alpha);
    ck_assert_int_eq(res, 0);

    res = uproc_word_from_string(&w2, s2, alpha);
    ck_assert_int_eq(res, 0);

    uproc_substmat *mat = uproc_substmat_eye();
    ck_assert_ptr_ne(mat, NULL);

    double dist[UPROC_SUFFIX_LEN] = {0};
    uproc_substmat_align_suffixes(mat, w1.suffix, w2.suffix, dist);
    for (int i = 0; i < UPROC_SUFFIX_LEN; i++) {
        double want = (i != 9);
        ck_assert_msg(dist[i] == want,
                      "wrong distance at index %i: got %g, want %g",
                      dist[i], want);
    }

    uproc_substmat_destroy(mat);
}
END_TEST

int main(void)
{
    (void)codon_is_stop;
    (void)codon_to_char;

    Suite *s = suite_create("substmat");

    TCase *tc = tcase_create("");
    tcase_add_checked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_align_suffixes);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

#include <string.h>
#include <ctype.h>

#include <check.h>
#include "uproc.h"
#include "../codon_tables.h"

uproc_codon codon_from_str(const char *s)
{
    uproc_codon c = 0;
    for (int i = 0; i < UPROC_CODON_NTS; i++) {
        uproc_nt nt = CHAR_TO_NT(s[i]);
        ck_assert(nt != -1);
        uproc_codon_append(&c, nt);
    }
    return c;
}

START_TEST(test_append_prepend)
{
    uproc_codon c1 = 0, c2 = 0;
    uproc_codon_append(&c1, CHAR_TO_NT('T'));
    uproc_codon_append(&c1, CHAR_TO_NT('C'));
    uproc_codon_append(&c1, CHAR_TO_NT('G'));

    uproc_codon_prepend(&c2, CHAR_TO_NT('G'));
    uproc_codon_prepend(&c2, CHAR_TO_NT('C'));
    uproc_codon_prepend(&c2, CHAR_TO_NT('T'));

    ck_assert(c1 == c2);
}
END_TEST

START_TEST(test_match)
{
    struct test_match {
        char *codon, *mask;
        bool match;
    } tests[] = {
        {"ACG", "ACN", true},
        {"AAG", "ARG", true},
        {"AGG", "ARG", true},
        {"ATG", "AYG", true},
        {"ACG", "AYG", true},
        {"AAA", "NRW", true},
        {"ACG", "ACT", false},
        {"ACN", "ACG", false},
    };

    for (int i = 0; i < sizeof tests / sizeof(struct test_match); i++) {
        struct test_match t = tests[i];
        bool m =
            uproc_codon_match(codon_from_str(t.codon), codon_from_str(t.mask));
        if (t.match) {
            ck_assert_msg(m, "%s matches %s", t.codon, t.mask);
        } else {
            ck_assert_msg(!m, "%s doesn't match %s", t.codon, t.mask);
        }
    }
}
END_TEST

START_TEST(test_complement)
{
    struct test_complement {
        char *c1, *c2;
        bool result;
    } tests[] = {
        {"ACG", "CGT", true},
        {"ARG", "CYT", true},
        {"NNA", "TNN", true},
        {"AAA", "TTT", true},
        {"CCC", "GGG", true},
        {"AAT", "TTA", false},
        {"GCG", "CCG", false},
    };

    for (int i = 0; i < sizeof tests / sizeof(struct test_complement); i++) {
        struct test_complement t = tests[i];
        uproc_codon c1 = codon_from_str(t.c1), c2 = codon_from_str(t.c2);

        if (t.result) {
            ck_assert_msg(c1 == CODON_COMPLEMENT(c2),
                          "%s is the complement of %s", t.c1, t.c2);
            ck_assert_msg(c2 == CODON_COMPLEMENT(c1),
                          "%s is the complement of %s", t.c2, t.c1);
        } else {
            ck_assert_msg(c1 != CODON_COMPLEMENT(c2),
                          "%s is NOT the complement of %s", t.c1, t.c2);
            ck_assert_msg(c2 != CODON_COMPLEMENT(c1),
                          "%s is NOT the complement of %s", t.c2, t.c1);
        }
    }
#undef TEST
}
END_TEST

int main(void)
{
    (void)codon_is_stop;
    (void)codon_to_char;

    Suite *s = suite_create("codon");

    TCase *tc = tcase_create("codon");
    tcase_add_test(tc, test_append_prepend);
    tcase_add_test(tc, test_complement);
    tcase_add_test(tc, test_match);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

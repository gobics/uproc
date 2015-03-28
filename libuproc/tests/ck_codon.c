#include <string.h>
#include <ctype.h>

#include "check_workaround.h"
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

START_TEST(test_match)
{
#define TEST(codon, mask) \
    ck_assert(uproc_codon_match(codon_from_str(codon), codon_from_str(mask)))
    TEST("ACG", "ACN");
    TEST("AAG", "ARG");
    TEST("AGG", "ARG");
    TEST("ATG", "AYG");
    TEST("ACG", "AYG");
    TEST("AAA", "NRW");
#undef TEST
}
END_TEST

START_TEST(test_complement)
{
#define TEST(a, b)                                                       \
    ck_assert(codon_from_str(a) == CODON_COMPLEMENT(codon_from_str(b))); \
    ck_assert(codon_from_str(b) == CODON_COMPLEMENT(codon_from_str(a)))

    TEST("ACG", "CGT");
    TEST("ARG", "CYT");
    TEST("NNA", "TNN");
    TEST("AAA", "TTT");
    TEST("CCC", "GGG");
#undef TEST
}
END_TEST

int main(void)
{
    (void)codon_is_stop;
    (void)codon_to_char;

    Suite *s = suite_create("codon");

    TCase *tc_compl = tcase_create("complement");
    tcase_add_test(tc_compl, test_complement);
    suite_add_tcase(s, tc_compl);

    TCase *tc_match = tcase_create("match");
    tcase_add_test(tc_match, test_match);
    suite_add_tcase(s, tc_match);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

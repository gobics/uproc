#include <string.h>
#include <ctype.h>

#include <check.h>
#include "uproc.h"
#include "../codon_tables.h"

#define ELEMENTS(x) (sizeof(x) / sizeof(x)[0])

#define assert_dbl_eq(a, b, name) do { \
    double _a = a, _b = b; \
    ck_assert_msg(fabs(_a - _b) <= UPROC_EPSILON, \
        "Assertion '%s' in test '%s' failed: %s == %g, %s == %g", \
        "|(" #a ")-(" #b ")| <= eps", (name), #a, _a, #b, _b, UPROC_EPSILON); \
    } while (0)

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

START_TEST(test_store_load)
{
    struct test {
        unsigned pos;
        uproc_amino a, b;
        double value;
    } tests[] = {
        {0, 0, 0, 51.5},
        {0, 10, 12, 51.5},
        {1, 11, 4, 29.2},
        {1, 11, 19, -20.25},
        {3, 11, 2, -2.5},
        {3, 15, 8, -14.75},
        {4, 1, 0, 57.3333},
        {4, 9, 19, 178},
        {8, 14, 19, -71},
        {11, 6, 8, -44},
        {11, 14, 9, 20.5},
    };
    uproc_substmat *mat = uproc_substmat_create();

    for (size_t i = 0; i < ELEMENTS(tests); i++) {
        struct test t = tests[i];
        ck_assert_int_ge(t.a, 0);
        ck_assert_int_ge(t.b, 0);
        uproc_substmat_set(mat, t.pos, t.a, t.b, t.value);
    }

    int res = uproc_substmat_store(mat, UPROC_IO_STDIO,
                               TMPDATADIR "test_substmat.tmp");
    ck_assert_msg(res == 0, "storing failed");
    uproc_substmat_destroy(mat);

    mat = uproc_substmat_load(UPROC_IO_STDIO, TMPDATADIR "test_substmat.tmp");
    ck_assert_ptr_ne(mat, NULL);

    for (size_t i = 0; i < ELEMENTS(tests); i++) {
        struct test t = tests[i];
        double got = uproc_substmat_get(mat, t.pos, t.a, t.b),
               want = t.value;

        char testname[1024];
        sprintf(testname, "i = %zu", i);
        assert_dbl_eq(got, want, testname);
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
    tcase_add_test(tc, test_store_load);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

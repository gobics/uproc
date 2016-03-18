#include <check.h>
#include "uproc.h"

uproc_alphabet *alpha;

#define assert_dbl_eq(a, b, name)                                     \
    do {                                                              \
        double _a = a, _b = b;                                        \
        ck_assert_msg(                                                \
            fabs(_a - _b) <= UPROC_EPSILON,                           \
            "Assertion '%s' in test '%s' failed: %s == %g, %s == %g", \
            "|(" #a ")-(" #b ")| <= eps", (name), #a, _a, #b, _b,     \
            UPROC_EPSILON);                                           \
    } while (0)

void setup(void)
{
    alpha = uproc_alphabet_create("AGSTPKRQEDNHYWFMLIVC");
}

void teardown(void)
{
    uproc_alphabet_destroy(alpha);
}

#define LEN(a) (sizeof(a) / sizeof *(a))

START_TEST(test_add)
{
    struct test_add {
        const char *name;
        struct test_add_word {
            size_t index;
            double d[UPROC_SUFFIX_LEN];
            enum uproc_ecurve_direction dir;
        } * words;
        size_t n_words;
        double expected;
    } tests[] = {
        {
            .name = "single word",
            .words =
                (struct test_add_word[]){
                    {
                        .d = {1.0, 2.0, 3.0}, .dir = UPROC_ECURVE_FWD,
                    },
                },
            .n_words = 1,
            .expected = 6.0,
        },
        {
            .name = "two non-overlapping",
            .words =
                (struct test_add_word[]){
                    {
                        .d = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0},
                        .dir = UPROC_ECURVE_FWD,
                    },
                    {
                        .index = 42,
                        .d = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0},
                        .dir = UPROC_ECURVE_FWD,
                    },
                },
            .n_words = 2,
            .expected = 42.0,
        },
        {
            .name = "two overlapping forward words",
            //      [ 0.0, 0.0, 1.0, -2.0, 3.0, 0.0, 0.0, ... ]
            // MAX  [ 0.0, 0.0, 0.0, -1.0, 2.0, 3.0, 0.0, ... ]
            // =    [ 0.0, 0.0, 1.0, -1.0, 3.0, 3.0, 0.0, ... ] == 6.0
            .words =
                (struct test_add_word[]){
                    {
                        .index = 2,
                        .d = {1.0, -2.0, 3.0},
                        .dir = UPROC_ECURVE_FWD,
                    },
                    {
                        .index = 3,
                        .d = {-1.0, 2.0, 3.0},
                        .dir = UPROC_ECURVE_FWD,
                    },
                },
            .n_words = 2,
            .expected = 6.0,
        },
        {
            .name = "two overlapping fwd and rev",
            //        0,    1,    2,    3,    4,    5,   6
            //    [-inf, -inf, -inf, -inf, -inf, -inf, 1.0, 2.0, 3.0, 0.0,  ...]
            //    [ 0.0,  0.0,  0.0,  0.0,  0.0,  3.0, 2.0, 1.0, 0.0, ..., -inf]
            // -> [ 0.0,  0.0,  0.0,  0.0,  0.0,  3.0, 2.0, 2.0, 3.0, 0.0,  ...]
            // == 10.0
            .words =
                (struct test_add_word[]){
                    {
                        .index = 0,
                        .d = {1.0, 2.0, 3.0},
                        .dir = UPROC_ECURVE_FWD,
                    },
                    {
                        .index = 5,
                        .d = {[9] = 1.0, 2.0, 3.0},
                        .dir = UPROC_ECURVE_REV,
                    },
                },
            .n_words = 2,
            .expected = 10.0,
        },
    };
    for (int t = 0; t < LEN(tests); t++) {
        struct test_add *test = &tests[t];
        uproc_mosaic *m = uproc_mosaic_create(true);
        for (int w = 0; w < test->n_words; w++) {
            struct test_add_word *word = &test->words[w];
            uproc_mosaic_add(m, NULL, word->index, word->d, word->dir);
        }
        assert_dbl_eq(uproc_mosaic_finalize(m), test->expected, test->name);
        uproc_mosaic_destroy(m);
    }
}
END_TEST

START_TEST(test_detailed)
{
    struct test_detailed {
        const char *name;
        struct test_detailed_word {
            char *str;
            size_t index;
            double d[UPROC_SUFFIX_LEN];
            enum uproc_ecurve_direction dir;
            double expected_score;
        } * words;
        size_t n_words;
    } tests[] = {
        {
            .name = "single word",
            .words =
                (struct test_detailed_word[]){
                    {
                        .str = "AAAAAAAAAAAAAAAAAA",
                        .index = 3,
                        .d = {1.0, 2.0, 3.0},
                        .dir = UPROC_ECURVE_FWD,
                        .expected_score = 6.0,
                    },
                },
            .n_words = 1,
        },
        {
            .name = "two words",
            .words =
                (struct test_detailed_word[]){
                    {
                        .str = "AAAAAAAAAAAAAAAAAA",
                        .d = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0},
                        .dir = UPROC_ECURVE_FWD,
                        .expected_score = 21.0,
                    },
                    {
                        .str = "PETERPETERPETERPET",
                        .index = 1,
                        .d =
                            {
                                1.0,
                            },
                        .dir = UPROC_ECURVE_FWD,
                        .expected_score = 1.0,
                    },
                },
            .n_words = 2,
        },
    };
    for (int t = 0; t < LEN(tests); t++) {
        struct test_detailed *test = &tests[t];
        uproc_mosaic *m = uproc_mosaic_create(true);
        fprintf(stderr, "%s\n", test->name);
        for (int w = 0; w < test->n_words; w++) {
            struct test_detailed_word *word = &test->words[w];
            struct uproc_word w;
            uproc_word_from_string(&w, word->str, alpha);
            uproc_mosaic_add(m, &w, word->index, word->d, word->dir);
        }
        uproc_list *mwords = uproc_mosaic_words_mv(m);
        ck_assert(mwords);
        ck_assert(!uproc_mosaic_words_mv(m));
        long n = uproc_list_size(mwords);
        ck_assert_int_eq(n, test->n_words);
        for (long i = 0; i < n; i++) {
            struct test_detailed_word *test_word = &test->words[i];
            struct uproc_mosaicword mw;
            uproc_list_get(mwords, i, &mw);
            char tmp[UPROC_WORD_LEN + 1];
            uproc_word_to_string(tmp, &mw.word, alpha);
            ck_assert_str_eq(tmp, test_word->str);
            ck_assert_int_eq(mw.index, test_word->index);
            ck_assert_int_eq(mw.dir, test_word->dir);
            assert_dbl_eq(mw.score, test_word->expected_score, test->name);
        }
        uproc_mosaic_destroy(m);
        uproc_list_destroy(mwords);
    }
}
END_TEST

int main(void)
{
    Suite *s = suite_create("mosaic");

    TCase *tc = tcase_create("mosaic");
    tcase_add_test(tc, test_add);
    tcase_add_test(tc, test_detailed);
    suite_add_tcase(s, tc);
    tcase_add_checked_fixture(tc, setup, teardown);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

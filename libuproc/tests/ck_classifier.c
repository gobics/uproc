#include <check.h>
#include <limits.h>
#include <string.h>
#include <inttypes.h>

#include "uproc.h"
#include "../classifier_internal.h"

#define assert_dbl_eq(a, b, name)                                     \
    do {                                                              \
        double _a = a, _b = b;                                        \
        ck_assert_msg(                                                \
            fabs(_a - _b) <= UPROC_EPSILON,                           \
            "Assertion '%s' in test '%s' failed: %s == %g, %s == %g", \
            "|(" #a ")-(" #b ")| <= eps", (name), #a, _a, #b, _b,     \
            UPROC_EPSILON);                                           \
    } while (0)

static uproc_list *make_mosaicword_list(struct uproc_mosaicword *words,
                                        size_t n)
{
    uproc_list *mw = uproc_list_create(sizeof(struct uproc_mosaicword));
    for (int i = 0; i < n; i++) {
        uproc_list_append(mw, &words[i]);
    }
    return mw;
}

int fake_classify(const void *context, const char *seq, uproc_list *results)
{
    (void)context;
    (void)seq;

    struct uproc_result fake_results[] = {
        {
            .rank = 1,
            .class = 2,
            .score = 42.0,
            .mosaicwords = make_mosaicword_list(
                (struct uproc_mosaicword[]){
                    {
                        .word = {0, 0}, .index = 0, .score = 3,
                    },
                    {
                        .word = {61414151, 74614151}, .index = 2, .score = 0.14,
                    },
                },
                2),
        },
        {
            .rank = 0, .class = 5, .score = 6.28,
        },
        {
            .rank = 0, .class = 1, .score = 3.14,
        },
    };
    int n = sizeof(fake_results) / sizeof(fake_results[0]);
    for (int i = 0; i < n; i++) {
        uproc_list_append(results, &fake_results[i]);
    }
    return 0;
}

void do_nothing(void *context)
{
    (void)context;
}

START_TEST(test_mode_max)
{
    uproc_clf *clf =
        uproc_clf_create(UPROC_CLF_MAX, NULL, fake_classify, do_nothing);
    ck_assert_ptr_ne(clf, NULL);

    uproc_list *results = NULL;

    ck_assert_int_eq(
        uproc_clf_classify(clf, "IMHHSGACEAKQQKNCHVFHWM", &results), 0);
    ck_assert_ptr_ne(results, NULL);

    ck_assert_int_eq(uproc_list_size(results), 1);

    struct uproc_result result;
    ck_assert_int_eq(uproc_list_get(results, 0, &result), 0);
    ck_assert_int_eq(result.rank, 0);
    ck_assert_int_eq(result.class, 5);
    assert_dbl_eq(result.score, 6.28, "test_mode_max");

    uproc_clf_destroy(clf);
    uproc_list_map2(results, uproc_result_freev);
    uproc_list_destroy(results);
}
END_TEST

int main(void)
{
    Suite *s = suite_create("classifier");

    TCase *tc = tcase_create("classifier");
    suite_add_tcase(s, tc);
    tcase_add_test(tc, test_mode_max);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

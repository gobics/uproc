#include <check.h>
#include "uproc.h"

uproc_list *list;

struct test_data
{
    int x;
    char c;
};

void setup(void)
{
    list = uproc_list_create(sizeof (struct test_data));
}

void teardown(void)
{
    uproc_list_destroy(list);
}

START_TEST(test_list)
{
    int res, i;
    struct test_data value;

    ck_assert_uint_eq(uproc_list_size(list), 0);

#define append(X, C) do {                       \
    value.x = (X); value.c = (C);               \
    int res = uproc_list_append(list, &value);  \
    ck_assert_int_eq(res, 0);                   \
} while (0)

    for (i = 0; i < 10000; i++) {
        append(i, '0' + i % 10);
    }
    ck_assert_uint_eq(uproc_list_size(list), 10000);
    append(42, 'a');
    append(7, 'c');

    ck_assert_uint_eq(uproc_list_size(list), 10002);

    for (i = 0; i < 10000; i++) {
        res = uproc_list_get(list, i, &value);
        ck_assert_int_eq(res, 0);
        ck_assert_int_eq(value.x, i);
    }

    uproc_list_get(list, 10000, &value);
    ck_assert_int_eq(value.x, 42);

    value.x = 1337;
    value.c = 'z';
    res = uproc_list_set(list, 42, &value);
    ck_assert_int_eq(res, 0);
    value.x = value.c = 'F';
    res = uproc_list_get(list, 42, &value);
    ck_assert_int_eq(value.x, 1337);
    ck_assert_int_eq(value.c, 'z');

    value.x = 114;
    value.c = '!';
    res = uproc_list_get(list, 10006, &value);
    ck_assert_int_eq(res, -1);
    ck_assert_int_eq(value.x, 114);
    ck_assert_int_eq(value.c, '!');
}
END_TEST

void
map_func_max(void *data, void *ctx)
{
    const struct test_data *p = data;
    int *max = ctx;
    if (p->x > *max) {
        *max = p->x;
    }
}

START_TEST(test_map)
{
    int i, max;
    struct test_data value;

    for (i = 0; i < 10000; i++) {
        if (i == 1042) {
            append(32521, '!');
        }
        append(i, '0' + i % 10);
    }

    uproc_list_map(list, map_func_max, &max);
    ck_assert_int_eq(max, 32521);
}
END_TEST

int main(void)
{
    Suite *s = suite_create("list");

    TCase *tc = tcase_create("list operations");
    tcase_add_test(tc, test_list);
    tcase_add_test(tc, test_map);
    tcase_add_checked_fixture(tc, setup, teardown);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

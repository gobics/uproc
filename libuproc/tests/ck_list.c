#include "check_workaround.h"
#include <limits.h>
#include <time.h>
#include "uproc.h"

#define TEST(I, V)                                   \
    do {                                             \
        struct test_data value;                      \
        int res = uproc_list_get(list, (I), &value); \
        ck_assert_int_eq(res, 0);                    \
        ck_assert_int_eq(value.x, (V));              \
    } while (0)

uproc_list *list;

struct test_data
{
    int x;
    char c;
};

void setup(void)
{
    list = uproc_list_create(sizeof(struct test_data));
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

#define APPEND(X, C)                               \
    do {                                           \
        value.x = (X);                             \
        value.c = (C);                             \
        int res = uproc_list_append(list, &value); \
        ck_assert_int_eq(res, 0);                  \
    } while (0)

    for (i = 0; i < 10000; i++) {
        APPEND(i, '0' + i % 10);
    }
    ck_assert_uint_eq(uproc_list_size(list), 10000);
    APPEND(42, 'a');
    APPEND(7, 'c');

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
    ck_assert_int_ne(res, 0);
    ck_assert_int_eq(value.x, 114);
    ck_assert_int_eq(value.c, '!');

    uproc_list_clear(list);
    res = uproc_list_get(list, 0, &value);
    ck_assert_int_ne(res, 0);
}
END_TEST

START_TEST(test_get_all)
{
    struct test_data values[] = {{1, '!'}, {42, 'A'}, {3, 'Z'}};
    uproc_list_append(list, &values[0]);
    uproc_list_append(list, &values[1]);
    memset(values, 0, sizeof values);
    uproc_list_get_all(list, values, sizeof values);
    ck_assert_int_eq(values[0].x, 1);
    ck_assert_int_eq(values[1].x, 42);
    ck_assert_int_eq(values[2].x, 0);
}
END_TEST

START_TEST(test_extend)
{
    int res;
    struct test_data values[] = {{0, '!'}, {42, 'A'}, {3, 'Z'}};

    res = uproc_list_extend(list, values, 3);
    ck_assert_int_eq(res, 0);

    res = uproc_list_extend(list, values, 3);
    ck_assert_int_eq(res, 0);

    ck_assert_int_eq(uproc_list_size(list), 6);

    TEST(0, 0);
    TEST(1, 42);
    TEST(3, 0);
    TEST(4, 42);
}
END_TEST

START_TEST(test_add)
{
    int res;
    struct test_data v, values[] = {{0, '!'}, {42, 'A'}, {3, 'Z'}};
    uproc_list_extend(list, values, 3);
    res = uproc_list_add(list, list);
    ck_assert_int_eq(res, 0);
    for (int i = 0; i < 3; i++) {
    uproc_list_get(list, i+3, &v);
        ck_assert_int_eq(v.x, values[i].x);
        ck_assert_int_eq(v.c, values[i].c);
    }
}
END_TEST

START_TEST(test_pop)
{
    int i;
    struct test_data value;
    for (i = 0; i < 10000; i++) {
        APPEND(i, '0' + i % 10);
    }
    ck_assert_int_eq(uproc_list_size(list), 10000);
    for (i = 0; i < 10000; i++) {
        uproc_list_pop(list, &value);
        ck_assert_int_eq(value.x, 10000 - i - 1);
    }
    ck_assert_int_eq(uproc_list_size(list), 0);
}
END_TEST

START_TEST(test_negative_index)
{
    int i, res;
    struct test_data value = {0, '!'};

    for (i = 0; i < 42; i++) {
        value.x = i;
        uproc_list_append(list, &value);
    }

    TEST(-1, 41);
    TEST(-2, 40);
    TEST(-10, 42 - 10);

    res = uproc_list_get(list, -43, &value);
    ck_assert_int_ne(res, 0);

    value.x = 1337;
    uproc_list_set(list, -3, &value);
    TEST(39, 1337);
#undef TEST
}
END_TEST

void map_func_max(void *data, void *ctx)
{
    const struct test_data *p = data;
    int *max = ctx;
    if (p->x > *max) {
        *max = p->x;
    }
}

START_TEST(test_map)
{
    int i, max = INT_MIN;
    struct test_data value;

    for (i = 0; i < 10000; i++) {
        if (i == 1042) {
            APPEND(32521, '!');
        }
        APPEND(i, '0' + i % 10);
    }

    uproc_list_map(list, map_func_max, &max);
    ck_assert_int_eq(max, 32521);
}
END_TEST

START_TEST(test_value_size_check)
{
    struct foo
    {
        int x[10];
    } foo = {{0}};
    int bar = 42;

    uproc_list *tmp = uproc_list_create(sizeof foo);
    ck_assert_int_eq(uproc_list_append(tmp, &foo), 0);
    ck_assert_int_ne(uproc_list_append(tmp, &bar), 0);

    ck_assert_int_eq(uproc_list_set(tmp, 0, &foo), 0);
    ck_assert_int_eq(uproc_list_get(tmp, 0, &foo), 0);
    ck_assert_int_eq(uproc_list_get_all(tmp, &foo, sizeof foo), sizeof foo);
    ck_assert_int_ne(uproc_list_set(tmp, 0, &bar), 0);
    uproc_list_destroy(tmp);
}
END_TEST

bool filter_u32_even(const void *data, void *ctx)
{
    (void)ctx;
    const uint32_t *p = data;
    return !(*p % 2);
}

START_TEST(test_filter)
{
    uint32_t value;
    uproc_list *list = uproc_list_create(sizeof value);

    for (uint32_t i = 0; i < 20; i++) {
        uproc_list_append(list, &i);
    }
    ck_assert_int_eq(uproc_list_size(list), 20);

    uproc_list_filter(list, filter_u32_even, NULL);

    ck_assert_int_eq(uproc_list_size(list), 10);

    while (uproc_list_size(list)) {
        uproc_list_pop(list, &value);
        ck_assert_uint_eq(value % 2, 0);
    }
}
END_TEST

int compare_int(const void *p1, const void *p2)
{
    // the list will contain only numbers from [0, RAND_MAX / 2), so overflow
    // is not possible
    const int *i1 = p1, *i2 = p2;
    return *i1 - *i2;
}

START_TEST(test_sort)
{
    srand(time(NULL));

    uproc_list *list = uproc_list_create(sizeof(int));

    for (int i = 0; i < 1337; i++) {
        int val = rand() / 2;
        uproc_list_append(list, &val);
    }

    uproc_list_sort(list, compare_int);
    int last_val = INT_MIN;
    for (long i = 0, n = uproc_list_size(list); i < n; i++) {
        int val;
        uproc_list_get(list, i, &val);
        ck_assert_int_le(last_val, val);
        last_val = val;
    }
}
END_TEST

START_TEST(test_max)
{
    srand(time(NULL));

    uproc_list *list = uproc_list_create(sizeof(int));

    for (int i = 0; i < 1337; i++) {
        int val = rand() / 2;
        uproc_list_append(list, &val);
    }

    int max;
    uproc_list_max(list, compare_int, &max);
    for (long i = 0, n = uproc_list_size(list); i < n; i++) {
        int val;
        uproc_list_get(list, i, &val);
        ck_assert_int_le(val, max);
    }
}
END_TEST

int main(void)
{
    Suite *s = suite_create("list");

    TCase *tc = tcase_create("list operations");
    tcase_add_test(tc, test_list);
    tcase_add_test(tc, test_get_all);
    tcase_add_test(tc, test_extend);
    tcase_add_test(tc, test_add);
    tcase_add_test(tc, test_pop);
    tcase_add_test(tc, test_negative_index);
    tcase_add_test(tc, test_map);
    tcase_add_test(tc, test_value_size_check);
    tcase_add_test(tc, test_filter);
    tcase_add_test(tc, test_sort);
    tcase_add_test(tc, test_max);
    tcase_add_checked_fixture(tc, setup, teardown);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

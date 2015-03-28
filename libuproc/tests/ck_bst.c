#include <check.h>
#include "uproc.h"

#define ELEMENTS(x) (sizeof(x) / sizoef(x)[0])

uproc_bst *bst;
union uproc_bst_key key;
struct test_data value;

#define N_TESTS 42

struct test_data
{
    int x;
    char c;
};

struct
{
    uintmax_t key;
    struct test_data value;
} test_data[] = {
      {155, {-15032, 't'}},
      {19, {4749, 'T'}},
      {226, {7059, 'g'}},
      {319, {-263, 'X'}},
      {302, {13290, 'l'}},
      {208, {-3764, 'U'}},
      {43, {14589, 'P'}},
      {137, {-7181, 'i'}},
      {332, {4586, 'o'}},
      {398, {5056, 'U'}},
      {407, {-2679, 'u'}},
      {355, {-2621, 'd'}},
      {89, {9937, 'Z'}},
      {357, {-6915, 'O'}},
      {341, {10515, 'X'}},
      {350, {6873, 'y'}},
      {156, {-1753, 'a'}},
      {45, {9183, 'U'}},
      {373, {-3602, 'p'}},
      {269, {5213, 'W'}},
      {209, {-3192, 'Q'}},
      {221, {201, 'l'}},
      {342, {-12152, 'c'}},
      {142, {-44, 'g'}},
      {85, {-1466, 'D'}},
      {233, {6565, 'F'}},
      {23, {9680, 'D'}},
      {44, {9615, 'P'}},
      {392, {-6078, 'T'}},
      {184, {-13062, 'l'}},
      {40, {11829, 'k'}},
      {122, {-8084, 'v'}},
      {168, {10444, 'T'}},
      {315, {14528, 'b'}},
      {212, {-8461, 'Q'}},
      {120, {3111, 'L'}},
      {3, {3752, 'h'}},
      {309, {-7457, 'i'}},
      {176, {3150, 'O'}},
      {397, {9754, 'A'}},
      {4, {-2353, 'D'}},
      {273, {3676, 'y'}},
};

unsigned idx[42] = {
    14, 17, 34, 6, 26, 39, 29, 40, 16, 7,  38, 19, 18, 21,
    22, 2,  9,  3, 28, 23, 0,  31, 15, 24, 12, 1,  32, 30,
    13, 4,  5,  8, 41, 27, 20, 36, 35, 33, 11, 10, 25, 37,
};

void setup(void)
{
    bst = uproc_bst_create(UPROC_BST_UINT, sizeof(struct test_data));
}

void teardown(void)
{
    uproc_bst_destroy(bst);
}

START_TEST(test_insert)
{
    int res, i, k;

    for (k = 0; k < N_TESTS; k++) {
        for (i = 0; i < k; i++) {
            key.uint = test_data[idx[i]].key;
            value = test_data[idx[i]].value;
            res = uproc_bst_insert(bst, key, &value);
            if (i == k - 1) {
                ck_assert_int_eq(res, 0);
            } else {
                ck_assert_int_eq(res, UPROC_BST_KEY_EXISTS);
            }
        }

        for (i = 0; i < N_TESTS; i++) {
            key.uint = test_data[idx[i]].key;
            res = uproc_bst_get(bst, key, &value);
            if (i < k) {
                ck_assert_int_eq(res, 0);
                ck_assert_int_eq(test_data[idx[i]].value.x, value.x);
                ck_assert_int_eq(test_data[idx[i]].value.c, value.c);
            } else {
                ck_assert_int_eq(res, UPROC_BST_KEY_NOT_FOUND);
            }
        }
    }
}
END_TEST

START_TEST(test_update)
{
    int res, i, k;
    for (k = 0; k < N_TESTS; k++) {
        for (i = 0; i < k; i++) {
            key.uint = test_data[idx[i]].key;
            value = test_data[idx[i]].value;
            value.x += k;

            res = uproc_bst_update(bst, key, &value);
            ck_assert_int_eq(res, 0);
        }

        for (i = 0; i < N_TESTS; i++) {
            key.uint = test_data[idx[i]].key;
            res = uproc_bst_get(bst, key, &value);
            if (i < k) {
                ck_assert_int_eq(res, 0);
                ck_assert_int_eq(test_data[idx[i]].value.x + k, value.x);
                ck_assert_int_eq(test_data[idx[i]].value.c, value.c);
            } else {
                ck_assert_int_eq(res, UPROC_BST_KEY_NOT_FOUND);
            }
        }
    }
}
END_TEST

START_TEST(test_iter)
{
    int res, i;
    uproc_bstiter *iter;
    uintmax_t last_key = 0;
    struct test_data last_value;

    for (i = 0; i < N_TESTS; i++) {
        key.uint = test_data[idx[i]].key;
        value = test_data[idx[i]].value;
        res = uproc_bst_update(bst, key, &value);
        ck_assert_int_eq(res, 0);
    }

    iter = uproc_bstiter_create(bst);

    for (i = 0; i < N_TESTS; i++) {
        res = uproc_bstiter_next(iter, &key, &value);
        ck_assert_int_eq(res, 0);
        if (i) {
            ck_assert_msg(key.uint > last_key,
                          "key.uint: %ju, lat_key: %ju", key.uint, last_key);
        }
        last_key = key.uint;
    }

    last_value = value;
    res = uproc_bstiter_next(iter, &key, &value);
    ck_assert_int_eq(res, 1);

    ck_assert_int_eq(key.uint, last_key);
    ck_assert_int_eq(value.x, last_value.x);
    ck_assert_int_eq(value.c, last_value.c);

    uproc_bstiter_destroy(iter);
}
END_TEST

int main(void)
{
    Suite *s = suite_create("bst");

    TCase *tc = tcase_create("bst operations");
    tcase_add_test(tc, test_insert);
    tcase_add_test(tc, test_update);
    tcase_add_test(tc, test_iter);
    tcase_add_checked_fixture(tc, setup, teardown);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

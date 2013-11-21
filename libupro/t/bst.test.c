#include "test.h"
#include "upro.h"

#include <stdlib.h>
#include <time.h>

struct upro_alphabet alpha;

struct test_data {
    int x;
    char c;
};

void setup(void)
{
    upro_alphabet_init(&alpha, ALPHABET);
}

void teardown(void)
{
}

int stuff(void)
{
    int res;
    struct upro_bst t;
    struct test_data value;
    union upro_bst_key key;

    DESC("inserting/updating");

    upro_bst_init(&t, UPRO_BST_UINT, sizeof value);

#define INS(k, d) \
    key.uint = k;   \
    value.x = d; \
    res = upro_bst_insert(&t, key, &value)

#define UPD(k, d) \
    key.uint = k;   \
    value.x = d; \
    res = upro_bst_update(&t, key, &value)

#define GET(k) \
    key.uint = k;   \
    res = upro_bst_get(&t, key, &value)

#define RM(k) \
    key.uint = k;   \
    res = upro_bst_remove(&t, key, NULL)

    INS(42, 0);
    assert_int_eq(res, UPRO_SUCCESS, "insertion succeeded");

    INS(21, 0);
    assert_int_eq(res, UPRO_SUCCESS, "insertion succeeded");

    INS(599, 0);
    assert_int_eq(res, UPRO_SUCCESS, "insertion succeeded");

    INS((unsigned long long) -1337, 42);
    assert_int_eq(res, UPRO_SUCCESS, "insertion succeeded");

    assert_int_eq(upro_bst_size(&t), 4, "upro_bst_size returns the right number of items\n");

    INS((unsigned long long) -1337, 0);
    assert_int_eq(res, UPRO_BST_KEY_EXISTS, "duplicate insertion failed");

    UPD(22, 32);
    assert_int_eq(res, UPRO_SUCCESS, "updating nonexistent key succeeded");

    UPD(21, 42);
    assert_int_eq(res, UPRO_SUCCESS, "updating existent key succeeded");

    assert_int_eq(upro_bst_size(&t), 5, "upro_bst_size returns the right number of items\n");

    GET(21);
    assert_int_eq(res, UPRO_SUCCESS, "getting existent key succeeded");
    assert_uint_eq(value.x, 42, "correct value retrieved");

    GET(42);
    assert_int_eq(res, UPRO_SUCCESS, "getting existent key succeeded");
    assert_uint_eq(value.x, 0, "correct value retrieved");

    RM(42);
    assert_int_eq(res, UPRO_SUCCESS, "removing existent key succeeded");

    GET(42);
    assert_int_eq(res, UPRO_BST_KEY_NOT_FOUND, "getting nonexistent key failed");

    GET(43);
    assert_int_eq(res, UPRO_BST_KEY_NOT_FOUND, "getting nonexistent key failed");

    RM(42);
    assert_int_eq(res, UPRO_BST_KEY_NOT_FOUND, "removing nonexistent key failed");

    upro_bst_clear(&t, NULL);

    return SUCCESS;
}

int iter(void)
{
    unsigned i;
    struct upro_bst t;
    struct upro_bstiter iter;
    union upro_bst_key key;
    uintmax_t value = 42;
    uintmax_t last;

    DESC("iterator");

    upro_bst_init(&t, UPRO_BST_UINT, sizeof value);

    for (i = 0; i < 10000; i++) {
        key.uint = rand();
        upro_bst_insert(&t, key, &value);
    }

    upro_bstiter_init(&iter, &t);
    upro_bstiter_next(&iter, &key, &value);
    last = key.uint;

    while (upro_bstiter_next(&iter, &key, &value) == UPRO_ITER_YIELD) {
        assert_uint_gt(key.uint, last, "iteration in correct order");
        last = key.uint;
    }
    upro_bst_clear(&t, NULL);
    return SUCCESS;
}

TESTS_INIT(stuff, iter);

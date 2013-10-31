#include "test.h"
#include "ecurve.h"

#include <stdlib.h>
#include <time.h>

struct ec_alphabet alpha;

struct test_data {
    int x;
    char c;
};

void setup(void)
{
    ec_alphabet_init(&alpha, ALPHABET);
}

void teardown(void)
{
}

int stuff(void)
{
    int res;
    struct ec_bst t;
    struct test_data value;
    union ec_bst_key key;

    DESC("inserting/updating");

    ec_bst_init(&t, EC_BST_UINT, sizeof value);

#define INS(k, d) \
    key.uint = k;   \
    value.x = d; \
    res = ec_bst_insert(&t, key, &value)

#define UPD(k, d) \
    key.uint = k;   \
    value.x = d; \
    res = ec_bst_update(&t, key, &value)

#define GET(k) \
    key.uint = k;   \
    res = ec_bst_get(&t, key, &value)

#define RM(k) \
    key.uint = k;   \
    res = ec_bst_remove(&t, key, NULL)

    INS(42, 0);
    assert_int_eq(res, EC_SUCCESS, "insertion succeeded");

    INS(21, 0);
    assert_int_eq(res, EC_SUCCESS, "insertion succeeded");

    INS(599, 0);
    assert_int_eq(res, EC_SUCCESS, "insertion succeeded");

    INS((unsigned long long) -1337, 42);
    assert_int_eq(res, EC_SUCCESS, "insertion succeeded");

    assert_int_eq(ec_bst_size(&t), 4, "ec_bst_size returns the right number of items\n");

    INS((unsigned long long) -1337, 0);
    assert_int_eq(res, EC_EEXIST, "duplicate insertion failed");

    UPD(22, 32);
    assert_int_eq(res, EC_SUCCESS, "updating nonexistent key succeeded");

    UPD(21, 42);
    assert_int_eq(res, EC_SUCCESS, "updating existent key succeeded");

    assert_int_eq(ec_bst_size(&t), 5, "ec_bst_size returns the right number of items\n");

    GET(21);
    assert_int_eq(res, EC_SUCCESS, "getting existent key succeeded");
    assert_uint_eq(value.x, 42, "correct value retrieved");

    GET(42);
    assert_int_eq(res, EC_SUCCESS, "getting existent key succeeded");
    assert_uint_eq(value.x, 0, "correct value retrieved");

    RM(42);
    assert_int_eq(res, EC_SUCCESS, "removing existent key succeeded");

    GET(42);
    assert_int_eq(res, EC_ENOENT, "getting nonexistent key failed");

    GET(43);
    assert_int_eq(res, EC_ENOENT, "getting nonexistent key failed");

    RM(42);
    assert_int_eq(res, EC_ENOENT, "removing nonexistent key failed");

    ec_bst_clear(&t, NULL);

    return SUCCESS;
}

int iter(void)
{
    unsigned i;
    struct ec_bst t;
    struct ec_bstiter iter;
    union ec_bst_key key;
    uintmax_t value = 42;
    uintmax_t last;

    DESC("iterator");

    ec_bst_init(&t, EC_BST_UINT, sizeof value);

    for (i = 0; i < 10000; i++) {
        key.uint = rand();
        ec_bst_insert(&t, key, &value);
    }

    ec_bstiter_init(&iter, &t);
    ec_bstiter_next(&iter, &key, &value);
    last = key.uint;

    while (ec_bstiter_next(&iter, &key, &value) == EC_ITER_YIELD) {
        assert_uint_gt(key.uint, last, "iteration in correct order");
        last = key.uint;
    }
    ec_bst_clear(&t, NULL);
    return SUCCESS;
}

TESTS_INIT(stuff, iter);

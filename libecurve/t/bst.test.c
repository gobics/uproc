#include "test.h"
#include "ecurve.h"

struct ec_alphabet alpha;

void setup(void)
{
    ec_alphabet_init(&alpha, EC_ALPHABET_ALPHA_DEFAULT);
}

void teardown(void)
{
}

int stuff(void)
{
    int res;
    struct ec_bst t;
    union ec_bst_key key;
    union ec_bst_data data;

    DESC("inserting/updating");

    ec_bst_init(&t, EC_BST_UINT);
    data.uint = 0;

#define INS(k, d) \
    key.uint = k;   \
    data.uint = d;  \
    res = ec_bst_insert(&t, key, data)

#define UPD(k, d) \
    key.uint = k;   \
    data.uint = d;  \
    res = ec_bst_update(&t, key, data)

#define GET(k) \
    key.uint = k;   \
    res = ec_bst_get(&t, key, &data)

#define RM(k) \
    key.uint = k;   \
    res = ec_bst_remove(&t, key, NULL)

    INS(42, 0);
    assert_int_eq(res, EC_SUCCESS, "insertion succeeded");

    INS(21, 0);
    assert_int_eq(res, EC_SUCCESS, "insertion succeeded");

    INS((unsigned long long) -1337, 42);
    assert_int_eq(res, EC_SUCCESS, "insertion succeeded");

    assert_int_eq(ec_bst_size(&t), 3, "ec_bst_size returns the right number of items\n");

    INS((unsigned long long) -1337, 0);
    assert_int_eq(res, EC_EEXIST, "duplicate insertion failed");

    UPD(22, 32);
    assert_int_eq(res, EC_SUCCESS, "updating nonexistent key succeeded");

    UPD(21, 42);
    assert_int_eq(res, EC_SUCCESS, "updating existent key succeeded");

    assert_int_eq(ec_bst_size(&t), 4, "ec_bst_size returns the right number of items\n");

    GET(21);
    assert_int_eq(res, EC_SUCCESS, "getting existent key succeeded");
    assert_uint_eq(data.uint, 42, "correct value retrieved");

    RM(42);
    assert_int_eq(res, EC_SUCCESS, "removing existent key succeeded");

    GET(42);
    assert_int_eq(res, EC_ENOENT, "getting nonexistent key failed");

    GET(43);
    assert_int_eq(res, EC_ENOENT, "getting nonexistent key failed");

    RM(42);
    assert_int_eq(res, EC_ENOENT, "removing nonexistent key failed");

    return SUCCESS;
}

TESTS_INIT(stuff);

#include "test.h"
#include "ecurve.h"

#undef EC_PREFIX_MAX
#define EC_PREFIX_MAX 100
#include "../src/ecurve.c"
#include "../src/word.c"

#define elements_of(a) (sizeof (a) / sizeof (a)[0])

ec_ecurve ecurve;
ec_suffix suffix_table[] = { 1, 3, 5, 10, 44, 131, 133, 1202, 4551ULL << (3 * EC_AMINO_BITS) };
ec_class class_table[] =   { 1, 0, 5,  0,  4,   1,   3,    2,    1 };


void teardown(void)
{
    free(ecurve.prefix_table);
}

void setup(void)
{
    unsigned i;
    ec_ecurve_init(&ecurve, 0);
    ecurve.suffix_count = elements_of(suffix_table);
    ecurve.suffix_table = suffix_table;
    ecurve.class_table = class_table;
    for (i = 0; i < 3; i++) {
        ecurve.prefix_table[i].first = NULL;
        ecurve.prefix_table[i].count = -1;
    }
    ecurve.prefix_table[3].first = &suffix_table[0];
    ecurve.prefix_table[3].count = 2;
    for (i = 4; i < 31; i++) {
        ecurve.prefix_table[i].first = &suffix_table[1];
        ecurve.prefix_table[i].count = 0;
    }
    ecurve.prefix_table[31].first = &suffix_table[2];
    ecurve.prefix_table[31].count = 3;
    for (i = 32; i < 53; i++) {
        ecurve.prefix_table[i].first = &suffix_table[4];
        ecurve.prefix_table[i].count = 0;
    }
    ecurve.prefix_table[53].first = &suffix_table[5];
    ecurve.prefix_table[53].count = 2;
    for (i = 54; i < 99; i++) {
        ecurve.prefix_table[i].first = &suffix_table[6];
        ecurve.prefix_table[i].count = 0;
    }
    ecurve.prefix_table[99].first = &suffix_table[7];
    ecurve.prefix_table[99].count = 2;

    ecurve.prefix_table[100].first = &suffix_table[8];
    ecurve.prefix_table[100].count = -1;
}


int test_suffix_lookup_val(ec_suffix x, ec_suffix lower, ec_suffix upper, int res)
{
    size_t lo, up;
    assert_int_eq(suffix_lookup(suffix_table, elements_of(suffix_table), x,
                                &lo, &up),
                  res, "got expected return value");
    assert_uint_eq(suffix_table[lo], lower, "got expected lower neighbour");
    assert_uint_eq(suffix_table[up], upper, "got expected upper neighbour");

    return SUCCESS;
}

int test_suffix_lookup(void)
{
    DESC("suffix_lookup()");
#define TEST(X, L, U, RES) do {                                 \
    if (test_suffix_lookup_val(X, L, U, RES) != SUCCESS) {      \
        return FAILURE;                                         \
    }                                                           \
} while (0)
    TEST(0, 1, 1, EC_LOOKUP_OOB);
    TEST(1, 1, 1, EC_LOOKUP_EXACT);
    TEST(2, 1, 3, EC_LOOKUP_INEXACT);
    TEST(3, 3, 3, EC_LOOKUP_EXACT);
    TEST(4, 3, 5, EC_LOOKUP_INEXACT);
    TEST(9, 5, 10, EC_LOOKUP_INEXACT);
    TEST(42, 10, 44, EC_LOOKUP_INEXACT);
    TEST(44, 44, 44, EC_LOOKUP_EXACT);
    TEST(131, 131, 131, EC_LOOKUP_EXACT);
    TEST(133, 133, 133, EC_LOOKUP_EXACT);
    TEST(134, 133, 1202, EC_LOOKUP_INEXACT);
#undef TEST
    return SUCCESS;
}


int test_prefix_lookup_val(ec_prefix key, ec_prefix l_prefix, ec_suffix *l_suffix,
                           ec_prefix u_prefix, ec_suffix *u_suffix, int res)
{
    ec_prefix lp, up;
    ec_suffix *ls, *us;

    assert_int_eq(prefix_lookup(ecurve.prefix_table, key, &lp, &ls, &up, &us),
                  res, "return value");

    assert_uint_eq(lp, l_prefix, "lower prefix");
    if (l_suffix) {
        assert_uint_eq(*ls, *l_suffix, "lower suffix");
    }
    assert_uint_eq(up, u_prefix, "upper prefix");
    if (u_suffix) {
        assert_uint_eq(*us, *u_suffix, "upper suffix");
    }

    return SUCCESS;
}

int test_prefix_lookup(void)
{
    DESC("prefix_lookup()");

#define TEST(X, LP, UP, LS, US, RES) do {                            \
    ec_suffix _ls = (LS), _us = (US);               \
    if (test_prefix_lookup_val(X, LP, &_ls, UP, &_us, RES) != SUCCESS) { \
        return FAILURE;                                         \
    }                                                           \
} while (0)

    TEST(0, 3, 3, suffix_table[0], suffix_table[0], EC_LOOKUP_OOB);
    TEST(3, 3, 3, suffix_table[0], suffix_table[1], EC_LOOKUP_EXACT);
    TEST(5, 3, 31, suffix_table[1], suffix_table[2], EC_LOOKUP_INEXACT);
    TEST(31, 31, 31, suffix_table[2], suffix_table[4], EC_LOOKUP_EXACT);
    TEST(32, 31, 53, suffix_table[4], suffix_table[5], EC_LOOKUP_INEXACT);
    TEST(52, 31, 53, suffix_table[4], suffix_table[5], EC_LOOKUP_INEXACT);
    TEST(53, 53, 53, suffix_table[5], suffix_table[6], EC_LOOKUP_EXACT);
    TEST(99, 99, 99, suffix_table[7], suffix_table[8], EC_LOOKUP_EXACT);
    TEST(100, 99, 99, suffix_table[8], suffix_table[8], EC_LOOKUP_OOB);
#undef TEST
    return SUCCESS;
}


int test_ecurve_lookup_val(struct ec_word w,
                           struct ec_word l_word,
                           struct ec_word u_word,
                           int res)
{
    struct ec_word lw, uw;
    ec_class lc, uc;

    INFO("w: %" EC_PREFIX_PRI " %" EC_SUFFIX_PRI, w.prefix, w.suffix);

    assert_int_eq(ec_ecurve_lookup(&ecurve, &w, &lw, &lc, &uw, &uc),
                  res, "return value");

    assert_uint_eq(lw.prefix, l_word.prefix, "lower prefix");
    assert_uint_eq(lw.suffix, l_word.suffix, "lower suffix");
    //assert_uint_eq(lc, l_class, "lower class");

    assert_uint_eq(uw.prefix, u_word.prefix, "upper prefix");
    assert_uint_eq(uw.suffix, u_word.suffix, "upper suffix");
    //assert_uint_eq(uc, u_class, "upper class");

    return SUCCESS;
}

int test_ecurve_lookup(void)
{
#define TEST(WP, WS, LP, LS, UP, US, RES) do {                              \
        struct ec_word _w = {WP, WS},                                       \
                       _l = {LP, LS},                                       \
                       _u = {UP, US};                                       \
        if (test_ecurve_lookup_val(_w, _l, _u, RES) != SUCCESS) {           \
            return FAILURE;                                                 \
        }                                                                   \
    } while (0)

    TEST(0, 1,  3, 1,  3, 1,  EC_LOOKUP_OOB);
    TEST(0, 44, 3, 1,  3, 1,  EC_LOOKUP_OOB);

    TEST(3, 1,  3, 1,  3, 1,  EC_LOOKUP_EXACT);
    TEST(3, 2,  3, 1,  3, 3,  EC_LOOKUP_INEXACT);

    TEST(31, 1,  31, 5,  31, 5,  EC_LOOKUP_INEXACT);
    TEST(31, 7,  31, 5,  31, 10,  EC_LOOKUP_INEXACT);
    TEST(31, 12,  31, 10,  31, 44,  EC_LOOKUP_INEXACT);

#define A (4551ULL << (3 * EC_AMINO_BITS))
    TEST(EC_PREFIX_MAX, 1,  99, A, 99, A, EC_LOOKUP_OOB);
    return SUCCESS;
}

TESTS_INIT(test_suffix_lookup, test_prefix_lookup, test_ecurve_lookup);

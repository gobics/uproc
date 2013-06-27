#include "test.h"
#include "ecurve.h"

#undef EC_PREFIX_MAX
#define EC_PREFIX_MAX 100
#include "../src/ecurve.c"
#include "../src/word.c"

#define elements_of(a) (sizeof (a) / sizeof (a)[0])

struct ec_ecurve ecurve;
ec_suffix suffix_table[] = { 1, 3, 5, 10, 44, 131, 133, 1202, 4551ULL << (3 * EC_AMINO_BITS) };
ec_class class_table[] =   { 1, 0, 5,  0,  4,   1,   3,    2,    1 };


void teardown(void)
{
    free(ecurve.prefix_table);
}

void setup(void)
{
    unsigned i;
    ec_ecurve_init(&ecurve, EC_ALPHABET_ALPHA_DEFAULT, 0);
    ecurve.suffix_count = elements_of(suffix_table);
    ecurve.suffix_table = suffix_table;
    ecurve.class_table = class_table;
    for (i = 0; i < 3; i++) {
        ecurve.prefix_table[i].first = 0;
        ecurve.prefix_table[i].count = -1;
    }
    ecurve.prefix_table[3].first = 0;
    ecurve.prefix_table[3].count = 2;
    for (i = 4; i < 31; i++) {
        ecurve.prefix_table[i].first = 1;
        ecurve.prefix_table[i].count = 0;
    }
    ecurve.prefix_table[31].first = 2;
    ecurve.prefix_table[31].count = 3;
    for (i = 32; i < 53; i++) {
        ecurve.prefix_table[i].first = 4;
        ecurve.prefix_table[i].count = 0;
    }
    ecurve.prefix_table[53].first = 5;
    ecurve.prefix_table[53].count = 2;
    for (i = 54; i < 99; i++) {
        ecurve.prefix_table[i].first = 6;
        ecurve.prefix_table[i].count = 0;
    }
    ecurve.prefix_table[99].first = 7;
    ecurve.prefix_table[99].count = 2;

    ecurve.prefix_table[100].first = 8;
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


int test_prefix_lookup_val(ec_prefix key, ec_prefix l_prefix, ec_prefix u_prefix,
                           size_t index, size_t count, int res)
{
    ec_prefix lp, up;
    size_t suff, c;

    INFO("key: %" EC_PREFIX_PRI, key);
    assert_int_eq(prefix_lookup(ecurve.prefix_table, key, &suff, &c, &lp, &up),
                  res, "return value");

    assert_uint_eq(lp, l_prefix, "lower prefix");
    assert_uint_eq(up, u_prefix, "upper prefix");

    assert_uint_eq(suff, index, "found index");
    assert_uint_eq(c, count, "found count");

    return SUCCESS;
}

int test_prefix_lookup(void)
{
    DESC("prefix_lookup()");

#define TEST(X, LP, UP, IDX, COUNT, RES) do {                            \
    if (test_prefix_lookup_val(X, LP, UP, IDX, COUNT, RES) != SUCCESS) { \
        return FAILURE;                                         \
    }                                                           \
} while (0)

    TEST(0, 3, 3,       0, 1, EC_LOOKUP_OOB);
    TEST(3, 3, 3,       0, 2, EC_LOOKUP_EXACT);
    TEST(5, 3, 31,      1, 2, EC_LOOKUP_INEXACT);
    TEST(31, 31, 31,    2, 3, EC_LOOKUP_EXACT);
    TEST(32, 31, 53,    4, 2, EC_LOOKUP_INEXACT);
    TEST(52, 31, 53,    4, 2, EC_LOOKUP_INEXACT);
    TEST(53, 53, 53,    5, 2, EC_LOOKUP_EXACT);
    TEST(99, 99, 99,    7, 2, EC_LOOKUP_EXACT);
    TEST(100, 99, 99,   8, 1, EC_LOOKUP_OOB);
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
    DESC("ecurve_lookup()");
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

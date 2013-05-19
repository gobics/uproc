#if HAVE_MMAP
#define _XOPEN_SOURCE 600
#endif

#include "test.h"
#include "ecurve.h"

#undef EC_PREFIX_MAX
#define EC_PREFIX_MAX 100
#if HAVE_MMAP
#include "../src/mmap.c"
#endif
#include "../src/ecurve.c"
#include "../src/storage.c"
#include "../src/word.c"

#define TMPFILE "t/ecurve.tmp"
#define elements_of(a) (sizeof (a) / sizeof (a)[0])

ec_ecurve ecurve;
ec_suffix suffix_table[] = { 1, 3, 5, 10, 44, 131, 133, 1202, 4551ULL << (3 * EC_AMINO_BITS) };
ec_class class_table[] =   { 1, 0, 5,  0,  4,   1,   3,    2,    1 };
ec_alphabet alpha;

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

void teardown(void)
{
    free(ecurve.prefix_table);
}

int test_binary_bufsz(void)
{
    DESC("buffer size for binary load/store");
    assert_uint_ge(BIN_BUFSZ, pack_bytes(BIN_HEADER_FMT), "buffer can fit header");
    assert_uint_ge(BIN_BUFSZ, pack_bytes(BIN_PREFIX_FMT), "buffer can fit prefix");
    assert_uint_ge(BIN_BUFSZ, pack_bytes(BIN_SUFFIX_FMT), "buffer can fit suffix");
    return SUCCESS;
}

int test_storage(int (*store)(), int (*load)())
{
    size_t i;
    ec_ecurve new_curve;

    assert_int_eq(ec_ecurve_store(&ecurve, TMPFILE, store),
                  EC_SUCCESS, "storing ecurve succeeded");
    assert_int_eq(ec_ecurve_load(&new_curve, TMPFILE, load),
                  EC_SUCCESS, "loading ecurve succeeded");

    for (i = 0; i < EC_PREFIX_MAX + 1; i++) {
        INFO("i = %zu", i);
        assert_uint_eq(ecurve.prefix_table[i].first, new_curve.prefix_table[i].first, "prefixes equal");
        assert_uint_eq(ecurve.prefix_table[i].count, new_curve.prefix_table[i].count, "prefixes equal");
    }

    assert_uint_eq(ecurve.suffix_count, new_curve.suffix_count, "suffix counts equal");
    for (i = 0; i < ecurve.suffix_count; i++)
    {
        INFO("i = %zu", i);
        assert_uint_eq(ecurve.suffix_table[i], new_curve.suffix_table[i], "suffixes equal");
        assert_uint_eq(ecurve.class_table[i], new_curve.class_table[i], "classes equal");
    }
    ec_ecurve_destroy(&new_curve);
    return SUCCESS;
}

int test_binary(void)
{
    DESC("writing and reading in binary format");
    return test_storage(ec_ecurve_store_binary, ec_ecurve_load_binary);
}

int test_plain(void)
{
    DESC("writing and reading in plain text");
    return test_storage(ec_ecurve_store_plain, ec_ecurve_load_plain);
}

int test_mmap(void)
{
#if HAVE_MMAP
    size_t i;
    ec_ecurve new_curve;

    DESC("mmapp()ing ecurve");

    assert_int_eq(ec_mmap_store(&ecurve, TMPFILE),
                  EC_SUCCESS, "storing ecurve succeeded");
    assert_int_eq(ec_mmap_map(&new_curve, TMPFILE),
                  EC_SUCCESS, "mmap()ing file succeeded");

    for (i = 0; i < EC_PREFIX_MAX + 1; i++) {
        INFO("i = %zu", i);
        assert_uint_eq(ecurve.prefix_table[i].first, new_curve.prefix_table[i].first, "prefixes equal");
        assert_uint_eq(ecurve.prefix_table[i].count, new_curve.prefix_table[i].count, "prefixes equal");
    }

    assert_uint_eq(ecurve.suffix_count, new_curve.suffix_count, "suffix counts equal");
    for (i = 0; i < ecurve.suffix_count; i++)
    {
        INFO("i = %zu", i);
        assert_uint_eq(ecurve.suffix_table[i], new_curve.suffix_table[i], "suffixes equal");
        assert_uint_eq(ecurve.class_table[i], new_curve.class_table[i], "classes equal");
    }
    ec_mmap_unmap(&new_curve);
    return SUCCESS;
#else
    DESC("mmapp()ing ecurve");
    SKIP("HAVE_MMAP is not defined");
#endif
}

TESTS_INIT(test_binary_bufsz, test_binary, test_plain, test_mmap);

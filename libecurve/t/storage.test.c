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

struct ec_ecurve ecurve;
ec_suffix suffixes[] = { 1, 3, 5, 10, 44, 131, 133, 1202, 4551ULL << (3 * EC_AMINO_BITS) };
ec_class classes[] =   { 1, 0, 5,  0,  4,   1,   3,    2,    1 };
struct ec_alphabet alpha;

void setup(void)
{
    unsigned i;
    ec_ecurve_init(&ecurve, EC_ALPHABET_ALPHA_DEFAULT, 0);
    ecurve.suffix_count = elements_of(suffixes);
    ecurve.suffixes = suffixes;
    ecurve.classes = classes;
    for (i = 0; i < 3; i++) {
        ecurve.prefixes[i].first = 0;
        ecurve.prefixes[i].count = -1;
    }
    ecurve.prefixes[3].first = 0;
    ecurve.prefixes[3].count = 2;
    for (i = 4; i < 31; i++) {
        ecurve.prefixes[i].first = 1;
        ecurve.prefixes[i].count = 0;
    }
    ecurve.prefixes[31].first = 2;
    ecurve.prefixes[31].count = 3;
    for (i = 32; i < 53; i++) {
        ecurve.prefixes[i].first = 4;
        ecurve.prefixes[i].count = 0;
    }
    ecurve.prefixes[53].first = 5;
    ecurve.prefixes[53].count = 2;
    for (i = 54; i < 99; i++) {
        ecurve.prefixes[i].first = 6;
        ecurve.prefixes[i].count = 0;
    }
    ecurve.prefixes[99].first = 7;
    ecurve.prefixes[99].count = 2;

    ecurve.prefixes[100].first = 8;
    ecurve.prefixes[100].count = -1;
}

void teardown(void)
{
    free(ecurve.prefixes);
}

int test_binary_bufsz(void)
{
    DESC("buffer size for binary load/store");
    assert_uint_ge(BIN_BUFSZ, pack_bytes(BIN_HEADER_FMT), "buffer can fit header");
    assert_uint_ge(BIN_BUFSZ, pack_bytes(BIN_PREFIX_FMT), "buffer can fit prefix");
    assert_uint_ge(BIN_BUFSZ, pack_bytes(BIN_SUFFIX_FMT), "buffer can fit suffix");
    return SUCCESS;
}

int test_storage(int format, char *filename)
{
    size_t i;
    struct ec_ecurve new_curve;

    assert_int_eq(ec_storage_store_file(&ecurve, filename, format),
                  EC_SUCCESS, "storing ecurve succeeded");
    assert_int_eq(ec_storage_load_file(&new_curve, filename, format),
                  EC_SUCCESS, "loading ecurve succeeded");

    for (i = 0; i < EC_PREFIX_MAX + 1; i++) {
        INFO("i = %zu", i);
        assert_uint_eq(ecurve.prefixes[i].first, new_curve.prefixes[i].first, "prefixes equal");
        assert_uint_eq(ecurve.prefixes[i].count, new_curve.prefixes[i].count, "prefixes equal");
    }

    assert_uint_eq(ecurve.suffix_count, new_curve.suffix_count, "suffix counts equal");
    for (i = 0; i < ecurve.suffix_count; i++)
    {
        INFO("i = %zu", i);
        assert_uint_eq(ecurve.suffixes[i], new_curve.suffixes[i], "suffixes equal");
        assert_uint_eq(ecurve.classes[i], new_curve.classes[i], "classes equal");
    }
    ec_ecurve_destroy(&new_curve);
    return SUCCESS;
}

int test_binary(void)
{
    DESC("writing and reading in binary format");
    return test_storage(EC_STORAGE_BINARY, TMPFILE ".bin");
}

int test_plain(void)
{
    DESC("writing and reading in plain text");
    return test_storage(EC_STORAGE_PLAIN, TMPFILE ".plain");
}

int test_mmap(void)
{
#if HAVE_MMAP
    size_t i;
    struct ec_ecurve new_curve;

    DESC("mmapp()ing ecurve");

    assert_int_eq(ec_mmap_store(&ecurve, TMPFILE ".mmap"),
                  EC_SUCCESS, "storing ecurve succeeded");
    assert_int_eq(ec_mmap_map(&new_curve, TMPFILE ".mmap"),
                  EC_SUCCESS, "mmap()ing file succeeded");

    for (i = 0; i < EC_PREFIX_MAX + 1; i++) {
        INFO("i = %zu", i);
        assert_uint_eq(ecurve.prefixes[i].first, new_curve.prefixes[i].first, "prefixes equal");
        assert_uint_eq(ecurve.prefixes[i].count, new_curve.prefixes[i].count, "prefixes equal");
    }

    assert_uint_eq(ecurve.suffix_count, new_curve.suffix_count, "suffix counts equal");
    for (i = 0; i < ecurve.suffix_count; i++)
    {
        INFO("i = %zu", i);
        assert_uint_eq(ecurve.suffixes[i], new_curve.suffixes[i], "suffixes equal");
        assert_uint_eq(ecurve.classes[i], new_curve.classes[i], "classes equal");
    }
    ec_mmap_unmap(&new_curve);
    return SUCCESS;
#else
    DESC("mmapp()ing ecurve");
    SKIP("HAVE_MMAP is not defined");
#endif
}

TESTS_INIT(test_binary_bufsz, test_binary, test_plain, test_mmap);

#if HAVE_MMAP
#define _XOPEN_SOURCE 600
#endif

#include "test.h"
#include "upro.h"

#undef UPRO_PREFIX_MAX
#define UPRO_PREFIX_MAX 100
#if HAVE_MMAP
#include "../src/mmap.c"
#endif
#include "../src/ecurve.c"
#include "../src/storage.c"
#include "../src/word.c"

#define TMPFILE "t/ecurve.tmp.%s"
#define elements_of(a) (sizeof (a) / sizeof (a)[0])

struct upro_ecurve ecurve;
upro_suffix suffixes[] = { 1, 3, 5, 10, 44, 131, 133, 1202, 4551ULL << (3 * UPRO_AMINO_BITS) };
upro_family families[] =   { 1, 0, 5,  0,  4,   1,   3,    2,    1 };
struct upro_alphabet alpha;

void setup(void)
{
    unsigned i;
    upro_ecurve_init(&ecurve, ALPHABET, 0);
    ecurve.suffix_count = elements_of(suffixes);
    ecurve.suffixes = suffixes;
    ecurve.families = families;
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

int test_storage(int format, int iotype, const char *ext)
{
    int res;
    size_t i;
    struct upro_ecurve new_curve;

    res = upro_storage_store(&ecurve, format, iotype, TMPFILE, ext);
    assert_int_eq(res, UPRO_SUCCESS, "storing ecurve succeeded");

    res = upro_storage_load(&new_curve, format, iotype, TMPFILE, ext);
    assert_int_eq(res, UPRO_SUCCESS, "loading ecurve succeeded");

    for (i = 0; i < UPRO_PREFIX_MAX + 1; i++) {
        INFO("i = %zu", i);
        assert_uint_eq(ecurve.prefixes[i].first, new_curve.prefixes[i].first, "prefixes equal");
        assert_uint_eq(ecurve.prefixes[i].count, new_curve.prefixes[i].count, "prefixes equal");
    }

    assert_uint_eq(ecurve.suffix_count, new_curve.suffix_count, "suffix counts equal");
    for (i = 0; i < ecurve.suffix_count; i++)
    {
        INFO("i = %zu", i);
        assert_uint_eq(ecurve.suffixes[i], new_curve.suffixes[i], "suffixes equal");
        assert_uint_eq(ecurve.families[i], new_curve.families[i], "families equal");
    }
    upro_ecurve_destroy(&new_curve);
    return SUCCESS;
}

int test_binary(void)
{
    DESC("writing and reading in binary format");
    return test_storage(UPRO_STORAGE_BINARY, UPRO_IO_STDIO, "bin");
}

int test_binary_gz(void)
{
    DESC("writing and reading in binary format (gzip)");
    return test_storage(UPRO_STORAGE_BINARY, UPRO_IO_GZIP, "bin.gz");
}

int test_plain(void)
{
    DESC("writing and reading in plain text");
    return test_storage(UPRO_STORAGE_PLAIN, UPRO_IO_STDIO, "plain");
}

int test_plain_gz(void)
{
    DESC("writing and reading in plain text (gzip)");
    return test_storage(UPRO_STORAGE_PLAIN, UPRO_IO_GZIP, "plain.gz");
}

int test_mmap(void)
{
#if HAVE_MMAP
    size_t i;
    int res;
    struct upro_ecurve new_curve;

    DESC("mmapp()ing ecurve");

    res = upro_storage_store(&ecurve, UPRO_STORAGE_MMAP, 0, TMPFILE, "mmap");
    assert_int_eq(res, UPRO_SUCCESS, "loading ecurve succeeded");

    res = upro_storage_load(&new_curve, UPRO_STORAGE_MMAP, 0, TMPFILE, "mmap");
    assert_int_eq(res, UPRO_SUCCESS, "loading ecurve succeeded");

    for (i = 0; i < UPRO_PREFIX_MAX + 1; i++) {
        INFO("i = %zu", i);
        assert_uint_eq(ecurve.prefixes[i].first, new_curve.prefixes[i].first, "prefixes equal");
        assert_uint_eq(ecurve.prefixes[i].count, new_curve.prefixes[i].count, "prefixes equal");
    }

    assert_uint_eq(ecurve.suffix_count, new_curve.suffix_count, "suffix counts equal");
    for (i = 0; i < ecurve.suffix_count; i++)
    {
        INFO("i = %zu", i);
        assert_uint_eq(ecurve.suffixes[i], new_curve.suffixes[i], "suffixes equal");
        assert_uint_eq(ecurve.families[i], new_curve.families[i], "families equal");
    }
    upro_mmap_unmap(&new_curve);
    return SUCCESS;
#else
    DESC("mmapp()ing ecurve");
    SKIP("HAVE_MMAP is not defined");
#endif
}

TESTS_INIT(test_binary,
        test_binary_gz,
        test_plain,
        test_plain_gz,
        test_mmap);

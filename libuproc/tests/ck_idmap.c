#include <stdlib.h>
#include <check.h>
#include "uproc.h"
#undef UPROC_FAMILY_MAX
#define UPROC_FAMILY_MAX 100
#include "../idmap.c"


uproc_idmap *map;

void setup(void)
{
    map = uproc_idmap_create();
}

void teardown(void)
{
    if (map) {
        uproc_idmap_destroy(map);
    }
}

START_TEST(idmap_usage)
{
    uproc_family fam1, fam2;

    fam1 = uproc_idmap_family(map, "foo");
    fam2 = uproc_idmap_family(map, "foo");
    ck_assert_int_eq(fam1, fam2);

    fam2 = uproc_idmap_family(map, "bar");
    ck_assert_int_ne(fam1, fam2);

    fam2 = uproc_idmap_family(map, "herp derp");
    ck_assert_int_ne(fam1, fam2);

    fam1 = uproc_idmap_family(map, "herp");
    ck_assert_int_ne(fam1, fam2);

    fam1 = uproc_idmap_family(map, "bar");
    ck_assert_int_ne(fam1, fam2);

    fam2 = uproc_idmap_family(map, "bar");
    ck_assert_int_eq(fam1, fam2);
}
END_TEST


START_TEST(idmap_exhaust)
{
    uproc_family fam, i;
    char foo[1024];
    for (i = 0; i < UPROC_FAMILY_MAX; i++) {
        sprintf(foo, "%" UPROC_FAMILY_PRI, i);
        fam = uproc_idmap_family(map, foo);
        ck_assert_int_eq(fam, i);
    }
    fam = uproc_idmap_family(map, "test1");
    ck_assert_int_eq(fam, UPROC_FAMILY_INVALID);
    fam = uproc_idmap_family(map, "test2");
    ck_assert_int_eq(fam, UPROC_FAMILY_INVALID);
    fam = uproc_idmap_family(map, "test3");
    ck_assert_int_eq(fam, UPROC_FAMILY_INVALID);
}
END_TEST



START_TEST(idmap_store_load)
{
    int res;
    uproc_family fam;

    uproc_idmap_family(map, "foo");
    uproc_idmap_family(map, "bar");
    uproc_idmap_family(map, "baz");
    uproc_idmap_family(map, "quux");
    uproc_idmap_family(map, "42");
    uproc_idmap_family(map, "herp derp");
    res = uproc_idmap_store(map, UPROC_IO_GZIP, DATADIR "test.idmap");
    ck_assert_msg(res == 0, "storing idmap failed");

    uproc_idmap_destroy(map);
    map = uproc_idmap_load(UPROC_IO_GZIP, DATADIR "test.idmap");
    ck_assert_ptr_ne(map, NULL);

    fam = uproc_idmap_family(map, "bar");
    ck_assert_int_eq(fam, 1);

    fam = uproc_idmap_family(map, "quux");
    ck_assert_int_eq(fam, 3);

    fam = uproc_idmap_family(map, "derp");
    ck_assert_int_eq(fam, 6);
}
END_TEST

START_TEST(idmap_load_invalid)
{
    uproc_idmap_destroy(map);
    map = NULL;
    map = uproc_idmap_load(UPROC_IO_GZIP, DATADIR "invalid_header.idmap");
    ck_assert_ptr_eq(map, NULL);

    map = uproc_idmap_load(UPROC_IO_GZIP, DATADIR "duplicate.idmap");
    ck_assert_ptr_eq(map, NULL);

    map = uproc_idmap_load(UPROC_IO_GZIP, DATADIR "missing_entry.idmap");
    ck_assert_ptr_eq(map, NULL);
}
END_TEST


Suite *
idmap_suite(void)
{
    Suite *s = suite_create("idmap");

    TCase *tc_idmap = tcase_create("idmap operations");
    tcase_add_checked_fixture(tc_idmap, setup, teardown);
    tcase_add_test(tc_idmap, idmap_usage);
    tcase_add_test(tc_idmap, idmap_exhaust);
    tcase_add_test(tc_idmap, idmap_store_load);
    tcase_add_test(tc_idmap, idmap_load_invalid);
    suite_add_tcase(s, tc_idmap);

    return s;
}

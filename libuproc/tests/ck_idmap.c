#include <stdlib.h>
#include <check.h>
#include "uproc.h"


struct uproc_idmap map;

void setup(void)
{
    uproc_idmap_init(&map);
}

void teardown(void)
{
    uproc_idmap_destroy(&map);
}

START_TEST(idmap_usage)
{
    uproc_family fam1, fam2;

    fam1 = uproc_idmap_family(&map, "foo");
    ck_assert_int_eq(map.n, 1);
    fam2 = uproc_idmap_family(&map, "foo");
    ck_assert_int_eq(map.n, 1);
    ck_assert_int_eq(fam1, fam2);

    fam2 = uproc_idmap_family(&map, "bar");
    ck_assert_int_ne(fam1, fam2);

    fam2 = uproc_idmap_family(&map, "herp derp");
    ck_assert_int_ne(fam1, fam2);

    fam1 = uproc_idmap_family(&map, "herp");
    ck_assert_int_ne(fam1, fam2);

    fam1 = uproc_idmap_family(&map, "bar");
    ck_assert_int_ne(fam1, fam2);

    fam2 = uproc_idmap_family(&map, "bar");
    ck_assert_int_eq(fam1, fam2);
}
END_TEST


START_TEST(idmap_exhaust)
{
    int res;
    uproc_family i;
    for (i = 0; i < UPROC_FAMILY_MAX; i++) {
        map.s[i] = "";
    }
    map.n = UPROC_FAMILY_MAX;
    res = uproc_idmap_family(&map, "test1");
    ck_assert_int_eq(res, UPROC_FAMILY_INVALID);
    res = uproc_idmap_family(&map, "test2");
    ck_assert_int_eq(res, UPROC_FAMILY_INVALID);
    res = uproc_idmap_family(&map, "test3");
    ck_assert_int_eq(res, UPROC_FAMILY_INVALID);
}
END_TEST



START_TEST(idmap_store_load)
{
    int res;
    uproc_family fam;

    uproc_idmap_family(&map, "foo");
    uproc_idmap_family(&map, "bar");
    uproc_idmap_family(&map, "baz");
    uproc_idmap_family(&map, "quux");
    uproc_idmap_family(&map, "42");
    uproc_idmap_family(&map, "herp derp");
    res = uproc_idmap_store(&map, UPROC_IO_GZIP, DATADIR "test.idmap");
    ck_assert_msg(res == 0, "storing idmap failed");

    uproc_idmap_destroy(&map);
    res = uproc_idmap_load(&map, UPROC_IO_GZIP, DATADIR "test.idmap");
    ck_assert_msg(res == 0, "loading idmap failed");

    fam = uproc_idmap_family(&map, "bar");
    ck_assert_int_eq(fam, 1);

    fam = uproc_idmap_family(&map, "quux");
    ck_assert_int_eq(fam, 3);

    fam = uproc_idmap_family(&map, "derp");
    ck_assert_int_eq(fam, 6);
}
END_TEST

START_TEST(idmap_load_invalid)
{
    int res;
    uproc_idmap_destroy(&map);
    res = uproc_idmap_load(&map, UPROC_IO_GZIP,
                           DATADIR "invalid_header.idmap");
    ck_assert_int_eq(res, -1);

    res = uproc_idmap_load(&map, UPROC_IO_GZIP, DATADIR "duplicate.idmap");
    ck_assert_int_eq(res, -1);

    res = uproc_idmap_load(&map, UPROC_IO_GZIP, DATADIR "missing_entry.idmap");
    ck_assert_int_eq(res, -1);
}
END_TEST


Suite *
idmap_suite(void)
{
    Suite *s = suite_create("idmap");

    TCase *tc_idmap = tcase_create("idmap operations");
    tcase_add_test(tc_idmap, idmap_usage);
    tcase_add_test(tc_idmap, idmap_exhaust);
    tcase_add_test(tc_idmap, idmap_store_load);
    tcase_add_test(tc_idmap, idmap_load_invalid);
    suite_add_tcase(s, tc_idmap);

    return s;
}

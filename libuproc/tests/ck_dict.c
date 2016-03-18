#include <check.h>
#include <limits.h>
#include <string.h>
#include <inttypes.h>
#include "uproc.h"

uproc_alphabet *alpha;

void setup(void)
{
    alpha = uproc_alphabet_create("AGSTPKRQEDNHYWFMLIVC");
    ck_assert(alpha);
}

void teardown(void)
{
    uproc_alphabet_destroy(alpha);
}

START_TEST(test_string_keys)
{
    uint64_t value = 0;
    uproc_dict *dict = uproc_dict_create(0, sizeof value);
    ck_assert(dict);

    ck_assert_int_ne(
        uproc_dict_set(
            dict, "string that is waaaaaaaay too long for using it as a key",
            &value),
        0);

    char key_barely_too_long[UPROC_DICT_KEY_SIZE_MAX + 1] = "";
    memset(key_barely_too_long, 'a', UPROC_DICT_KEY_SIZE_MAX);
    ck_assert_int_ne(uproc_dict_set(dict, key_barely_too_long, &value), 0);

    char key_barely_short_enough[UPROC_DICT_KEY_SIZE_MAX] = "";
    memset(key_barely_too_long, 'a', UPROC_DICT_KEY_SIZE_MAX - 1);
    ck_assert_int_eq(uproc_dict_set(dict, key_barely_short_enough, &value), 0);

    // empty string is a valid key
    ck_assert_int_eq(uproc_dict_set(dict, "", &value), 0);

    // insert two items
    value = 42;
    ck_assert_int_eq(uproc_dict_set(dict, "spam", &value), 0);
    value = 1337;
    ck_assert_int_eq(uproc_dict_set(dict, "egg", &value), 0);

    // verify their stored value
    ck_assert_int_eq(uproc_dict_get(dict, "spam", &value), 0);
    ck_assert_uint_eq(value, 42);

    ck_assert_int_eq(uproc_dict_get(dict, "egg", &value), 0);
    ck_assert_uint_eq(value, 1337);

    // remove spam
    ck_assert_int_eq(uproc_dict_remove(dict, "spam"), 0);
    // make sure it is gone
    ck_assert_int_eq(uproc_dict_get(dict, "spam", &value),
                     UPROC_DICT_KEY_NOT_FOUND);
    // make sure egg is still there
    ck_assert_int_eq(uproc_dict_get(dict, "egg", &value), 0);

    uproc_dict_clear(dict);
    ck_assert_uint_eq(uproc_dict_size(dict), 0);
    ck_assert_int_eq(uproc_dict_get(dict, "egg", &value),
                     UPROC_DICT_KEY_NOT_FOUND);
    uproc_dict_destroy(dict);
}
END_TEST

START_TEST(test_many_values)
{
    uint64_t value = 0;

    char *line = NULL;
    size_t line_sz;

    uproc_dict *dict = uproc_dict_create(0, sizeof value);
    ck_assert(dict);

    // insert all the lines
    {
        uproc_io_stream *f = uproc_io_open("r", UPROC_IO_STDIO,
                                           DATADIR "dict_english_wordlist.txt");
        ck_assert(f);
        value = 0;
        while (uproc_io_getline(&line, &line_sz, f) != -1) {
            *strchr(line, '\n') = '\0';
            ck_assert_int_eq(uproc_dict_set(dict, line, &value), 0);
            value++;
        }
        uproc_io_close(f);
        ck_assert_uint_eq(uproc_dict_size(dict), value);
    }

    // Retrieve all the keys
    {
        uproc_io_stream *f = uproc_io_open("r", UPROC_IO_STDIO,
                                           DATADIR "dict_english_wordlist.txt");
        ck_assert(f);
        value = 0;
        while (uproc_io_getline(&line, &line_sz, f) != -1) {
            *strchr(line, '\n') = '\0';
            uint64_t tmp;
            ck_assert_int_eq(uproc_dict_get(dict, line, &tmp), 0);
            ck_assert_uint_eq(tmp, value);
            value++;
        }
        uproc_io_close(f);
    }

    // Set new value for a key (hand-picked from the list)
    {
        long old_size = uproc_dict_size(dict);
        ck_assert_int_eq(uproc_dict_get(dict, "wombat", &value), 0);
        value = 1;
        uproc_dict_set(dict, "wombat", &value);
        value = 114151;
        uproc_dict_get(dict, "wombat", &value);
        ck_assert_uint_eq(value, 1);
        ck_assert_int_eq(uproc_dict_size(dict), old_size);
    }

    // Remove half of the keys, verify that all the others are still there
    {
        uproc_io_stream *f = uproc_io_open("r", UPROC_IO_STDIO,
                                           DATADIR "dict_english_wordlist.txt");
        size_t line_number = 0;
        ck_assert(f);
        value = 0;
        while (uproc_io_getline(&line, &line_sz, f) != -1) {
            *strchr(line, '\n') = '\0';
            if (line_number++ <= 29617 / 2) {
                ck_assert_int_eq(uproc_dict_remove(dict, line), 0);
                ck_assert_int_eq(uproc_dict_get(dict, line, &value),
                                 UPROC_DICT_KEY_NOT_FOUND);
            } else {
                ck_assert_int_eq(uproc_dict_get(dict, line, &value), 0);
            }
        }
        uproc_io_close(f);
        ck_assert_int_eq(uproc_dict_size(dict), 29617 / 2);
    }
    uproc_dict_destroy(dict);
    free(line);
}
END_TEST

START_TEST(test_update)
{
    struct {
        const char *key, *value;
    } a[] =
        {
            {"foo", "bar"}, {"bacon", "aarhgad"},
        },
      b[] =
          {
              {"spam", "egg"}, {"bacon", "beans"},
          },
      want[] = {
          {"foo", "bar"}, {"spam", "egg"}, {"bacon", "beans"},
      };

#define ELEMENTS(x) (sizeof(x) / sizeof(x)[0])

    uproc_dict *dict_a = uproc_dict_create(0, 0),
               *dict_b = uproc_dict_create(0, 0),
               *dict_want = uproc_dict_create(0, 0);

    for (size_t i = 0, n = ELEMENTS(a); i < n; i++) {
        uproc_dict_set(dict_a, a[i].key, a[i].value);
    }
    for (size_t i = 0, n = ELEMENTS(b); i < n; i++) {
        uproc_dict_set(dict_b, b[i].key, b[i].value);
    }

    uproc_dict_update(dict_a, dict_b);

    for (size_t i = 0, n = ELEMENTS(want); i < n; i++) {
        char value[UPROC_DICT_VALUE_SIZE_MAX];
        int res = uproc_dict_get(dict_a, want[i].key, &value);
        if (res) {
            uproc_perror("");
            ck_assert_int_eq(res, 0);
        }
        ck_assert_str_eq(value, want[i].value);
    }
}
END_TEST

START_TEST(test_iter)
{
    struct uproc_word key;
    memset(&key, 0, sizeof key);
    key = (struct uproc_word)UPROC_WORD_INITIALIZER;

    char value[UPROC_WORD_LEN + 1] = "";

    uproc_dict *dict = uproc_dict_create(sizeof key, sizeof value);
    ck_assert(dict);

    char *line = NULL;
    size_t line_sz;
    uproc_io_stream *f =
        uproc_io_open("r", UPROC_IO_STDIO, DATADIR "dict_single_words.txt");
    ck_assert(f);
    while (uproc_io_getline(&line, &line_sz, f) != -1) {
        uproc_word_from_string(&key, line, alpha);
        strncpy(value, line, UPROC_WORD_LEN);

        // the list contains unique values only, get should fail
        ck_assert_int_eq(uproc_dict_get(dict, &key, &value),
                         UPROC_DICT_KEY_NOT_FOUND);

        // set should succeed
        ck_assert_int_eq(uproc_dict_set(dict, &key, &value), 0);
    }
    uproc_io_close(f);
    ck_assert_int_eq(uproc_dict_size(dict), 2000);

    // now we iterate
    long iterations = 0;
    uproc_dictiter *iter = uproc_dictiter_create(dict);
    int res;
    struct uproc_word last_key = UPROC_WORD_INITIALIZER;
    memset(&last_key, 0, sizeof last_key);
    while (res = uproc_dictiter_next(iter, &key, &value), !res) {
        char tmp[UPROC_WORD_LEN + 1] = "";
        uproc_word_to_string(tmp, &key, alpha);
        ck_assert_str_eq(tmp, value);
        iterations++;
        ck_assert_int_ne(uproc_word_cmp(&last_key, &key), 0);
        last_key = key;
    }
    ck_assert_int_ne(res, -1);
    uproc_dictiter_destroy(iter);
    ck_assert_int_eq(uproc_dict_size(dict), iterations);
    uproc_dict_destroy(dict);
    free(line);
}
END_TEST

void map_func(void *key, void *value, void *opaque)
{
    int *k = key;
    uint64_t *v = value, *o = opaque;
    ck_assert_uint_eq(o[*k], *v);
    o[*k] = 0;
}

START_TEST(test_map)
{
    int key = 0;
    uint64_t values[] = {42, 1337, 31};
    uproc_dict *dict = uproc_dict_create(sizeof key, sizeof values[0]);
    ck_assert(dict);

    for (int key = 0; key < 3; key++) {
        ck_assert_int_eq(uproc_dict_set(dict, &key, &values[key]), 0);
    }

    uproc_dict_map(dict, map_func, values);

    for (int key = 0; key < 3; key++) {
        ck_assert_uint_eq(values[key], 0);
    }
    uproc_dict_destroy(dict);
}
END_TEST

int format(char *str, const void *key, const void *value, void *opaque)
{
    (void)opaque;
    const uint64_t *v = value;
    int res = snprintf(str, UPROC_DICT_STORE_BUFFER_SIZE - 1, "%s %" PRIu64,
                       (char *)key, *v);
    return res > 0 ? 0 : -1;
}

int scan(const char *str, void *key, void *value, void *opaque)
{
    (void)opaque;
    uint64_t v = 0;
    int res = sscanf(str, "%s %" SCNu64, (char *)key, &v);
    memcpy(value, &v, sizeof v);
    return res == 2 ? 0 : -1;
}

START_TEST(test_store_load)
{
    uint64_t value = 0;
    uproc_dict *dict = uproc_dict_create(0, sizeof value);
    ck_assert(dict);
    uproc_dict_set(dict, "spam", &value);
    value = 42;
    uproc_dict_set(dict, "egg", &value);
    value = 1337;
    uproc_dict_set(dict, "bacon", &value);
    ck_assert_uint_eq(uproc_dict_size(dict), 3);

    int res = uproc_dict_store(dict, format, NULL, UPROC_IO_GZIP,
                               TMPDATADIR "test_dict.tmp");
    ck_assert_int_eq(res, 0);
    uproc_dict_destroy(dict);

    dict = uproc_dict_load(0, sizeof value, scan, NULL, UPROC_IO_GZIP,
                           TMPDATADIR "test_dict.tmp");
    ck_assert(dict);
    ck_assert_uint_eq(uproc_dict_size(dict), 3);
    uproc_dict_get(dict, "spam", &value);
    ck_assert_uint_eq(value, 0);
    uproc_dict_get(dict, "egg", &value);
    ck_assert_uint_eq(value, 42);
    uproc_dict_get(dict, "bacon", &value);
    ck_assert_uint_eq(value, 1337);

    uproc_dict_destroy(dict);
}
END_TEST

int main(void)
{
    Suite *s = suite_create("dict");

    TCase *tc = tcase_create("dict stuff");
    tcase_add_test(tc, test_string_keys);
    tcase_add_test(tc, test_many_values);
    tcase_add_test(tc, test_update);
    tcase_add_test(tc, test_iter);
    tcase_add_test(tc, test_map);
    tcase_add_test(tc, test_store_load);
    tcase_add_checked_fixture(tc, setup, teardown);
    suite_add_tcase(s, tc);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int n_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return n_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

#ifndef EC_TEST_H
#define EC_TEST_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define PATH_PREFIX "t/"

typedef int (*test)(void);

enum {
    SUCCESS, FAILURE
};

extern char *test_desc, test_error[];
extern test tests[];
#define TESTS_INIT(...) test tests[] = { __VA_ARGS__, NULL }

#define TAPDIAG "# "


#define FAIL(...) sprintf(test_error, __VA_ARGS__); return FAILURE


/* ASSERTION MACROS */

#define assert_xint_op(X, OP, Y, DESC, type, fmt) do {                      \
    type _test_x = (X), _test_y = (Y);                                      \
    if (!(_test_x OP _test_y)) {                                            \
        FAIL("Assertion '" DESC " (" #X " " #OP " " #Y ")' failed:\n"       \
             TAPDIAG "  " #X ":\n"                                          \
             TAPDIAG "    " fmt "\n"                                        \
             TAPDIAG "  " #Y ":\n"                                          \
             TAPDIAG "    " fmt "\n",                                       \
             _test_x, _test_y);                                             \
    }                                                                       \
} while (0)

#define assert_int_op(X, OP, Y, DESC) assert_xint_op(X, OP, Y, DESC, intmax_t, "%jd")
#define assert_uint_op(X, OP, Y, DESC) assert_xint_op(X, OP, Y, DESC, uintmax_t, "%ju")

#define assert_int_eq(X, Y, DESC) assert_int_op(X, ==, Y, DESC)
#define assert_int_ne(X, Y, DESC) assert_int_op(X, !=, Y, DESC)
#define assert_int_ge(X, Y, DESC) assert_int_op(X, >=, Y, DESC)
#define assert_int_gt(X, Y, DESC) assert_int_op(X, >, Y, DESC)
#define assert_int_le(X, Y, DESC) assert_int_op(X, <=, Y, DESC)
#define assert_int_lt(X, Y, DESC) assert_int_op(X, <, Y, DESC)

#define assert_uint_eq(X, Y, DESC) assert_uint_op(X, ==, Y, DESC)
#define assert_uint_ne(X, Y, DESC) assert_uint_op(X, !=, Y, DESC)
#define assert_uint_ge(X, Y, DESC) assert_uint_op(X, >=, Y, DESC)
#define assert_uint_gt(X, Y, DESC) assert_uint_op(X, >, Y, DESC)
#define assert_uint_le(X, Y, DESC) assert_uint_op(X, <=, Y, DESC)
#define assert_uint_lt(X, Y, DESC) assert_uint_op(X, <, Y, DESC)

#define assert_strcmp(X, OP, Y, DESC) do {                              \
    char *_test_x = (X), *_test_y = (Y);                                \
    int _test_res = strcmp(_test_x, _test_y);                           \
    if (!(_test_res OP 0)) {                                            \
        FAIL("Assertion '" DESC                                         \
             " (strcmp(" #X ", " #Y ") " #OP " 0)' failed:\n"           \
             TAPDIAG "  " #X ":\n"                                      \
             TAPDIAG "    %s\n"                                         \
             TAPDIAG "  " #Y ":\n"                                      \
             TAPDIAG "    %s\n",                                        \
             _test_x, _test_y);                                         \
    }                                                                   \
} while (0)

#define assert_str_eq(X, Y, DESC) assert_strcmp(X, ==, Y, DESC)



void setup(void);
void teardown(void);
#endif

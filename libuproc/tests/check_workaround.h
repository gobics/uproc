#include <check.h>
#if CHECK_MAJOR_VERSION == 0 && CHECK_MINOR_VERSION == 9 && CHECK_MICRO_VERSION < 10
/* check 0.9.6 (which travis uses) evaluates the given expressions twice … */
#undef ck_assert_int_eq
#define ck_assert_int_eq(X, Y) ck_assert(X == Y)
#undef ck_assert_int_ne
#define ck_assert_int_ne(X, Y) ck_assert(X != Y)

/* the following were not present before 0.9.10 */
#define ck_assert_ptr_eq(X, Y) ck_assert(X == Y)
#define ck_assert_ptr_ne(X, Y) ck_assert(X != Y)
#define ck_assert_uint_eq(X, Y) ck_assert(X == Y)
#define ck_assert_uint_gt(X, Y) ck_assert(X > Y)
#define ck_assert_int_le(X, Y) ck_assert(X <= Y)
#endif

#include <math.h>

#include "test.h"
#include "upro.h"

void setup(void)
{
}

void teardown(void)
{
}

int sizes(void)
{
    DESC("sizes of types");
    assert_uint_eq(UPRO_PREFIX_MAX, pow(UPRO_ALPHABET_SIZE, UPRO_PREFIX_LEN) - 1,
                   "value of UPRO_PREFIX_MAX");
    assert_uint_gt(~(upro_prefix)0, UPRO_PREFIX_MAX,
                   "upro_prefix large enough (can store UPRO_PREFIX_MAX + 1)");
    assert_uint_ge(sizeof (upro_suffix) * CHAR_BIT, (UPRO_AMINO_BITS * UPRO_SUFFIX_LEN),
                   "upro_suffix large enough");
    return SUCCESS;
}


TESTS_INIT(sizes);

#include <math.h>

#include "test.h"
#include "ecurve.h"

void setup(void)
{
}

void teardown(void)
{
}

int sizes(void)
{
    DESC("sizes of types");
    assert_uint_eq(EC_PREFIX_MAX, pow(EC_ALPHABET_SIZE, EC_PREFIX_LEN) - 1,
                   "value of EC_PREFIX_MAX");
    assert_uint_gt(~(ec_prefix)0, EC_PREFIX_MAX,
                   "ec_prefix large enough (can store EC_PREFIX_MAX + 1)");
    assert_uint_ge(sizeof (ec_suffix) * CHAR_BIT, (EC_AMINO_BITS * EC_SUFFIX_LEN),
                   "ec_suffix large enough");
    return SUCCESS;
}


TESTS_INIT(sizes);

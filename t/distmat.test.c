#include "test.h"
#include "ecurve.h"

struct ec_alphabet alpha;

void setup(void)
{
    ec_alphabet_init(&alpha, EC_ALPHABET_ALPHA_DEFAULT);
}

void teardown(void)
{
}

int blosum(void)
{
    int i, k, res;
    struct ec_distmat mat;
    DESC("reading BLOSUM matrix");

    res = ec_distmat_load(&mat, PATH_PREFIX "IDENTITY", ec_distmat_load_blosum, &alpha);
    assert_int_eq(res, EC_SUCCESS, "opening file succeeded");

    for (i = 0; i < EC_ALPHABET_SIZE; i++) {
        for (k = 0; k < EC_ALPHABET_SIZE; k++) {
            assert_int_eq(ec_distmat_get(&mat, i, k), ec_distmat_get(&mat, k, i),
                          "scores are symmetric");
            if (k == i) {
                assert_int_eq(ec_distmat_get(&mat, i, k), 1,
                              "identity score");
            }
            else {
                assert_int_eq(ec_distmat_get(&mat, i, k), -10000,
                              "non-identity score");
            }
        }
    }
    return SUCCESS;
}

int blosum_twice(void)
{
    int i, k, res;
    struct ec_distmat mat[2];
    DESC("reading two BLOSUM matrices from one file");

    res = ec_distmat_load_many(mat, 2, PATH_PREFIX "IDENTITY_TWICE",
                               ec_distmat_load_blosum, &alpha);
    assert_int_eq(res, EC_SUCCESS, "opening file succeeded");

    for (i = 0; i < EC_ALPHABET_SIZE; i++) {
        for (k = 0; k < EC_ALPHABET_SIZE; k++) {
            assert_int_eq(ec_distmat_get(&mat[0], i, k), ec_distmat_get(&mat[1], i, k),
                          "matrix entries are identical");
        }
    }
    return SUCCESS;
}

TESTS_INIT(blosum, blosum_twice);

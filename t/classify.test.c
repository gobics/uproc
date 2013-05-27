#include <math.h>

#include "test.h"
#include "ecurve.h"
#include "../src/classify.c"

void setup(void)
{
}

void teardown(void)
{
}

int sc_add_finalize(void)
{
    DESC("sc_add and sc_finalize");
    struct sc score = SC_INITIALIZER;
    ec_dist dist[EC_SUFFIX_LEN] = { 0.0 };

    dist[0] = 0.5;
    sc_add(&score, 1, dist);
    assert_double_eq(score.total, 0.0, "total score");
    assert_uint_eq(score.index, 1, "index");

    dist[0] = 1.0;
    sc_add(&score, 1, dist);
    assert_double_eq(score.total, 0.0, "total score");
    assert_uint_eq(score.index, 1, "index");

    dist[0] = 1.0;
    dist[1] = 3.0;
    sc_add(&score, 2, dist);
    assert_double_eq(score.total, 1.0, "total score");
    assert_uint_eq(score.index, 2, "index");

    dist[0] = dist[1] = 0.0;
    sc_add(&score, 4, dist);
    assert_double_eq(score.total, 5.0, "total score");
    assert_uint_eq(score.index, 4, "index");

    assert_double_eq(sc_finalize(&score), 5.0, "final score");
    return SUCCESS;
}

int scores_add_finalize(void)
{
    ec_bst scores;
    ec_dist dist[EC_SUFFIX_LEN] = { 0.0 };
    ec_dist predict_score;
    ec_class predict_cls;

    DESC("scores_add and scores_finalize");
    ec_bst_init(&scores);

    dist[4] = 1.0;
    scores_add(&scores, 10, 0, dist);
    scores_add(&scores, 23, 0, dist);
    scores_add(&scores, 33, 1, dist);
    scores_add(&scores, 42, 0, dist);
    scores_add(&scores, 42, 1, dist);
    scores_add(&scores, 13, 0, dist);
    scores_add(&scores, 13, 1, dist);
    scores_add(&scores, 42, 2, dist);
    dist[4] = 0;
    scores_add(&scores, 42, 2, dist);

    scores_finalize(&scores, &predict_cls, &predict_score);
    assert_double_eq(predict_score, 3.0, "'predicted' score");
    assert_uint_eq(predict_cls, 42, "'predicted' class");

    return SUCCESS;
}


TESTS_INIT(sc_add_finalize, scores_add_finalize);

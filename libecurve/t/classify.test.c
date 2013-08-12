#include <math.h>
#include <string.h>

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
    struct sc score;
    double dist[EC_SUFFIX_LEN] = { 0.0 };

    sc_init(&score);
    dist[0] = 0.5;
    dist[1] = -1.0;
    sc_add(&score, 1, dist, false);
    assert_double_eq(score.total, 0.0, "total score");
    assert_uint_eq(score.index, 1, "index");

    dist[0] = 0.0;
    sc_add(&score, 1, dist, false);
    assert_double_eq(score.total, 0.0, "total score");
    assert_uint_eq(score.index, 1, "index");

    dist[EC_SUFFIX_LEN - EC_PREFIX_LEN - 1] = 1.2;
    sc_add(&score, 1, dist, true);

    memcpy(dist, (double[EC_SUFFIX_LEN]){ 0.0 }, sizeof dist);
    dist[11] = 2.2;
    sc_add(&score, 1, dist, true);
    assert_double_eq(score.total, 0.0, "total score");
    assert_uint_eq(score.index, 1, "index");

    dist[0] = 1.0;
    dist[1] = 3.0;
    dist[11] = 0.0;
    sc_add(&score, 8, dist, false);
    assert_double_eq(score.total, 3.4, "total score");
    assert_uint_eq(score.index, 8, "index");

    memcpy(dist, (double[EC_SUFFIX_LEN]){ 0.0 }, sizeof dist);
    sc_add(&score, 16, dist, false);
    assert_double_eq(score.total, 7.4, "total score");
    assert_uint_eq(score.index, 16, "index");

    assert_double_eq(sc_finalize(&score), 7.4, "final score");

    sc_init(&score);
    dist[0] = 1.0;
    dist[1] = -3.0;
    sc_add(&score, 0, dist, false);
    assert_double_eq(sc_finalize(&score), -2.0, "negative total score");

    sc_init(&score);
    memcpy(dist, (double[EC_SUFFIX_LEN]){ 0.0 }, sizeof dist);
    dist[0] = 1.0;
    dist[1] = 0.5;
    sc_add(&score, 0, dist, false);
    sc_add(&score, 1, dist, false);
    sc_add(&score, 2, dist, false);
    dist[0] = dist[1] = 0.0;
    dist[4] = 2.0;
    sc_add(&score, 2, dist, true);
    assert_double_eq(sc_finalize(&score), 5.0, "correct score with overlaps");

    return SUCCESS;
}

int scores_add_finalize(void)
{
    struct ec_bst scores;
    double dist[EC_SUFFIX_LEN] = { 0.0 };
    double predict_score;
    ec_class predict_cls;

    DESC("scores_add and scores_finalize");
    ec_bst_init(&scores);

    dist[4] = 1.0;
    scores_add(&scores, 10, 0, dist, false);
    scores_add(&scores, 23, 0, dist, false);
    scores_add(&scores, 33, 1, dist, false);
    scores_add(&scores, 42, 0, dist, false);
    scores_add(&scores, 42, 1, dist, false);
    scores_add(&scores, 13, 0, dist, false);
    scores_add(&scores, 13, 1, dist, false);
    scores_add(&scores, 42, 2, dist, true);
    dist[4] = 0;
    scores_add(&scores, 42, 2, dist, false);
    scores_add(&scores, 42, 2, dist, true);

    finalize_max(&scores, &predict_cls, &predict_score);
    assert_double_eq(predict_score, 3.0, "'predicted' score");
    assert_uint_eq(predict_cls, 42, "'predicted' class");

    return SUCCESS;
}


TESTS_INIT(sc_add_finalize, scores_add_finalize);

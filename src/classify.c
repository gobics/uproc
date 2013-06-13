#include <stdlib.h>
#include <assert.h>

#include "ecurve/common.h"
#include "ecurve/bst.h"
#include "ecurve/ecurve.h"
#include "ecurve/distmat.h"

struct sc {
    size_t index;
    ec_dist total, dist[EC_SUFFIX_LEN];
};

struct sc_max {
    ec_dist score;
    ec_class cls;
};


static void
sc_init(struct sc *s)
{
    size_t i;
    s->index = -1;
    s->total = 0.0;
    for (i = 0; i < EC_SUFFIX_LEN; i++) {
        s->dist[i] = EC_DIST_MIN;
    }
}

static struct sc *
sc_new(void)
{
    struct sc *s = malloc(sizeof *s);
    if (s) {
        sc_init(s);
    }
    return s;
}

static void
sc_add(struct sc *score, size_t index, ec_dist dist[static EC_SUFFIX_LEN])
{
    size_t i, diff;

    if (score->index == (size_t) -1) {
        diff = 0;
    }
    else {
        assert(index >= score->index);
        diff = index - score->index;
        for (i = 0; i < diff; i++) {
            score->total += score->dist[i];
        }
    }

    for (i = 0; i + diff < EC_SUFFIX_LEN; i++) {
#define MAX(a, b) (a > b ? a : b)
        score->dist[i] = MAX(score->dist[i + diff], dist[i]);
    }
    for (; i < EC_SUFFIX_LEN; i++) {
        score->dist[i] = dist[i];
    }
    score->index = index;
}

static ec_dist
sc_finalize(struct sc *score)
{
    size_t i;
    for (i = 0; i < EC_SUFFIX_LEN; i++) {
        score->total += score->dist[i];
    }
    return score->total;
}

static int
scores_finalize_cb(intmax_t key, void *data, void *opaque)
{
    ec_class cls = key;
    ec_dist s = sc_finalize(data);
    struct sc_max *m = opaque;
    if (s > m->score) {
        m->score = s;
        m->cls = cls;
    }
    return EC_SUCCESS;
}

static int
scores_finalize(ec_bst *scores, ec_class *predict_cls, ec_dist *predict_score)
{
    int res;
    struct sc_max m = { EC_DIST_MIN, -1 };
    res = ec_bst_walk(scores, &scores_finalize_cb, &m);
    if (res == EC_SUCCESS) {
        *predict_cls = m.cls;
        *predict_score = m.score;
    }
    return res;
}

static int
scores_add(ec_bst *scores, ec_class cls, size_t index,
           ec_dist dist[static EC_SUFFIX_LEN])
{
    struct sc *s = ec_bst_get(scores, cls);
    if (!s) {
        if (!(s = sc_new())) {
            return EC_FAILURE;
        }
        if (ec_bst_insert(scores, cls, s) != EC_SUCCESS) {
            return EC_FAILURE;
        }
    }
    sc_add(s, index, dist);
    return EC_SUCCESS;
}

static void
align_suffixes(ec_dist dist[static EC_SUFFIX_LEN], ec_suffix s1, ec_suffix s2,
               const ec_distmat distmat[static EC_SUFFIX_LEN])
{
    size_t i;
    ec_amino a1, a2;
    for (i = 0; i < EC_SUFFIX_LEN; i++) {
        a1 = s1 & EC_BITMASK(EC_AMINO_BITS);
        a2 = s2 & EC_BITMASK(EC_AMINO_BITS);
        s1 >>= EC_AMINO_BITS;
        s2 >>= EC_AMINO_BITS;
        dist[i] = ec_distmat_get(&distmat[i], a1, a2);
    }
}

static int
scores_add_word(ec_bst *scores, const struct ec_word *word, size_t index,
                const ec_ecurve *ecurve,
                const ec_distmat distmat[static EC_SUFFIX_LEN])
{
    int res;
    struct ec_word
        lower_nb = EC_WORD_INITIALIZER,
        upper_nb = EC_WORD_INITIALIZER;
    ec_class lower_cls, upper_cls;
    ec_dist dist[EC_SUFFIX_LEN];

    if (!ecurve) {
        return EC_SUCCESS;
    }
    ec_ecurve_lookup(ecurve, word, &lower_nb, &lower_cls, &upper_nb, &upper_cls);

    align_suffixes(dist, word->suffix, lower_nb.suffix, distmat);
    res = scores_add(scores, lower_cls, index, dist);
    if (res != EC_SUCCESS || ec_word_equal(&lower_nb, &upper_nb)) {
        return res;
    }
    align_suffixes(dist, word->suffix, upper_nb.suffix, distmat);
    res = scores_add(scores, upper_cls, index, dist);
    return res;
}

int
ec_classify(const char *seq, const ec_distmat distmat[static EC_SUFFIX_LEN],
            const ec_ecurve *fwd_ecurve, const ec_ecurve *rev_ecurve,
            ec_class *predict_cls, ec_dist *predict_score)
{
    int res;
    ec_bst scores;
    ec_alphabet alpha;
    ec_worditer iter;
    size_t index;
    struct ec_word
        fwd_word = EC_WORD_INITIALIZER,
        rev_word = EC_WORD_INITIALIZER;

    ec_bst_init(&scores);
    ec_ecurve_get_alphabet(fwd_ecurve ? fwd_ecurve : rev_ecurve, &alpha);
    ec_worditer_init(&iter, seq, &alpha);

    while (ec_worditer_next(&iter, &index, &fwd_word, &rev_word) == EC_SUCCESS) {
        res = scores_add_word(&scores, &fwd_word, index, fwd_ecurve, distmat);
        if (res != EC_SUCCESS) {
            goto error;
        }
        res = scores_add_word(&scores, &rev_word, index, rev_ecurve, distmat);
        if (res != EC_SUCCESS) {
            goto error;
        }
    }
    res = scores_finalize(&scores, predict_cls, predict_score);

error:
    ec_bst_clear(&scores, &free);
    return res;
}

#include <stdlib.h>
#include <assert.h>

#include "ecurve/common.h"
#include "ecurve/bst.h"
#include "ecurve/ecurve.h"
#include "ecurve/substmat.h"
#include "ecurve/orf.h"
#include "ecurve/matrix.h"
#include "ecurve/classify.h"

struct sc {
    size_t index;
    double total, dist[EC_WORD_LEN];
};

struct sc_max {
    double score;
    ec_class cls;
};


static void
sc_init(struct sc *s)
{
    size_t i;
    s->index = -1;
    s->total = 0.0;
    for (i = 0; i < EC_WORD_LEN; i++) {
        s->dist[i] = -INFINITY;
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
sc_add(struct sc *score, size_t index, double dist[static EC_SUFFIX_LEN],
       bool reverse)
{
    size_t i, diff, offset;

    if (score->index != (size_t) -1) {
        assert(index >= score->index);
        diff = index - score->index;
        if (diff > EC_SUFFIX_LEN) {
            diff = EC_SUFFIX_LEN;
        }
        for (i = 0; i < diff; i++) {
            if (isfinite(score->dist[i])) {
                score->total += score->dist[i];
                score->dist[i] = -INFINITY;
            }
        }
    }
    else {
        diff = 0;
    }

    offset = reverse ? 0 : EC_PREFIX_LEN;

    for (i = 0; i + diff < EC_SUFFIX_LEN; i++) {
#define MAX(a, b) (a > b ? a : b)
        score->dist[i + offset] = MAX(score->dist[i + offset + diff],
                                      dist[reverse ? EC_SUFFIX_LEN - i - 1 : i]);
    }
    for (; i < EC_SUFFIX_LEN; i++) {
        score->dist[i + offset] = dist[i];
    }
    score->index = index;
}

static double
sc_finalize(struct sc *score)
{
    size_t i;
    for (i = 0; i < EC_SUFFIX_LEN; i++) {
        if (isfinite(score->dist[i])) {
            score->total += score->dist[i];
        }
    }
    return score->total;
}

static int
scores_finalize_cb(intmax_t key, void *data, void *opaque)
{
    ec_class cls = key;
    double s = sc_finalize(data);
    struct sc_max *m = opaque;
    if (s > m->score) {
        m->score = s;
        m->cls = cls;
    }
    return EC_SUCCESS;
}

static int
scores_finalize(struct ec_bst *scores, ec_class *predict_cls, double *predict_score)
{
    int res;
    struct sc_max m = { -INFINITY, -1 };
    res = ec_bst_walk(scores, &scores_finalize_cb, &m);
    if (res == EC_SUCCESS) {
        *predict_cls = m.cls;
        *predict_score = m.score;
    }
    return res;
}

static int
scores_add(struct ec_bst *scores, ec_class cls, size_t index,
           double dist[static EC_SUFFIX_LEN], bool reverse)
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
    sc_add(s, index, dist, reverse);
    return EC_SUCCESS;
}

static void
align_suffixes(double dist[static EC_SUFFIX_LEN], ec_suffix s1, ec_suffix s2,
               const struct ec_substmat substmat[static EC_SUFFIX_LEN])
{
    size_t i;
    ec_amino a1, a2;
    for (i = 0; i < EC_SUFFIX_LEN; i++) {
        a1 = s1 & EC_BITMASK(EC_AMINO_BITS);
        a2 = s2 & EC_BITMASK(EC_AMINO_BITS);
        s1 >>= EC_AMINO_BITS;
        s2 >>= EC_AMINO_BITS;
        dist[i] = ec_substmat_get(&substmat[i], a1, a2);
    }
}

static int
scores_add_word(struct ec_bst *scores, const struct ec_word *word, size_t index,
                bool reverse, const struct ec_ecurve *ecurve,
                const struct ec_substmat substmat[static EC_SUFFIX_LEN])
{
    int res;
    struct ec_word
        lower_nb = EC_WORD_INITIALIZER,
        upper_nb = EC_WORD_INITIALIZER;
    ec_class lower_cls, upper_cls;
    double dist[EC_SUFFIX_LEN];

    if (!ecurve) {
        return EC_SUCCESS;
    }
    ec_ecurve_lookup(ecurve, word, &lower_nb, &lower_cls, &upper_nb, &upper_cls);

    align_suffixes(dist, word->suffix, lower_nb.suffix, substmat);
    res = scores_add(scores, lower_cls, index, dist, reverse);
    if (res != EC_SUCCESS || ec_word_equal(&lower_nb, &upper_nb)) {
        return res;
    }
    align_suffixes(dist, word->suffix, upper_nb.suffix, substmat);
    res = scores_add(scores, upper_cls, index, dist, reverse);
    return res;
}

int
ec_classify_protein(const char *seq,
                    const struct ec_substmat substmat[static EC_SUFFIX_LEN],
                    const struct ec_ecurve *fwd_ecurve,
                    const struct ec_ecurve *rev_ecurve,
                    ec_class *predict_cls,
                    double *predict_score)
{
    int res;
    struct ec_bst scores;
    struct ec_worditer iter;
    struct ec_alphabet alpha;
    size_t index;
    struct ec_word
        fwd_word = EC_WORD_INITIALIZER,
        rev_word = EC_WORD_INITIALIZER;

    ec_bst_init(&scores);
    ec_ecurve_get_alphabet(fwd_ecurve ? fwd_ecurve : rev_ecurve, &alpha);
    ec_worditer_init(&iter, seq, &alpha);

    while (ec_worditer_next(&iter, &index, &fwd_word, &rev_word) == EC_SUCCESS) {
        res = scores_add_word(&scores, &fwd_word, index, false, fwd_ecurve, substmat);
        if (res != EC_SUCCESS) {
            goto error;
        }
        res = scores_add_word(&scores, &rev_word, index, true, rev_ecurve, substmat);
        if (res != EC_SUCCESS) {
            goto error;
        }
    }
    res = EC_SUCCESS;
    if (ec_bst_isempty(&scores)) {
        *predict_score = -INFINITY;
    }
    else {
        scores_finalize(&scores, predict_cls, predict_score);
    }

error:
    ec_bst_clear(&scores, &free);
    return res;
}

int
ec_classify_dna(const char *seq,
                enum ec_orf_mode mode,
                const struct ec_orf_codonscores *codon_scores,
                const struct ec_matrix *thresholds,
                const struct ec_substmat substmat[static EC_SUFFIX_LEN],
                const struct ec_ecurve *fwd_ecurve,
                const struct ec_ecurve *rev_ecurve,
                ec_class *predict_cls,
                double *predict_score)
{
    int res;
    unsigned i;
    char *orf[EC_ORF_FRAMES] = { NULL };
    size_t orf_sz[EC_ORF_FRAMES];

    res = ec_orf_chained(seq, mode, codon_scores, thresholds, orf, orf_sz);
    if (res != EC_SUCCESS) {
        goto error;
    }

    for (i = 0; i < mode; i++) {
        if (orf[i]) {
            res = ec_classify_protein(orf[i], substmat, fwd_ecurve, rev_ecurve,
                                      &predict_cls[i], &predict_score[i]);
            if (res != EC_SUCCESS) {
                goto error;
            }
        }
        else {
            predict_cls[i] = -1;
            predict_score[i] = -INFINITY;
        }
    }

error:
    for (i = 0; i < mode; i++) {
        free(orf[i]);
    }
    return res;
}

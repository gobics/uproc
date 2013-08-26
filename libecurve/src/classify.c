#include <stdlib.h>
#include <string.h>

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
reverse_array(void *p, size_t n, size_t sz)
{
    unsigned char *s = p, tmp;
    size_t i, k, i1, i2;

    for (i = 0; i < n / 2; i++) {
        for (k = 0; k < sz; k++) {
            i1 = sz * i + k;
            i2 = sz * (n - i - 1) + k;
            tmp = s[i1];
            s[i1] = s[i2];
            s[i2] = tmp;
        }
    }
}

static void
sc_add(struct sc *score, size_t index, double dist[static EC_SUFFIX_LEN],
       bool reverse)
{
    size_t i, diff;
    double tmp[EC_WORD_LEN];

    for (i = 0; i < EC_PREFIX_LEN; i++) {
        tmp[i] = -INFINITY;
    }
    memcpy(tmp + EC_PREFIX_LEN, dist, sizeof *dist * EC_SUFFIX_LEN);
    if (reverse) {
        reverse_array(tmp, EC_WORD_LEN, sizeof *tmp);
    }

    if (score->index != (size_t) -1) {
        diff = index - score->index;
        if (diff > EC_WORD_LEN) {
            diff = EC_WORD_LEN;
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

    for (i = 0; i + diff < EC_WORD_LEN; i++) {
#define MAX(a, b) (a > b ? a : b)
        score->dist[i] = MAX(score->dist[i + diff], tmp[i]);
    }
    for (; i < EC_WORD_LEN; i++) {
        score->dist[i] = tmp[i];
    }
    score->index = index;
}

static double
sc_finalize(struct sc *score)
{
    size_t i;
    for (i = 0; i < EC_WORD_LEN; i++) {
        if (isfinite(score->dist[i])) {
            score->total += score->dist[i];
        }
    }
    return score->total;
}

struct all_cb_context {
    struct {
        double score;
        ec_class cls;
    } *preds;
    size_t i;
};

static int
finalize_all_cb(union ec_bst_key key, union ec_bst_data data, void *opaque)
{
    struct all_cb_context *ctx = opaque;
    double score = sc_finalize(data.ptr);
    if (score > 0.0) {
        ctx->preds[ctx->i].cls = key.uint;
        ctx->preds[ctx->i].score = score;
        ctx->i++;
    }
    return EC_SUCCESS;
}

static int
finalize_all(struct ec_bst *score_tree, size_t *n,
                    ec_class **predict_cls, double **predict_scores)
{
    int res = EC_SUCCESS;
    struct all_cb_context ctx;
    size_t i, count;

    ctx.i = 0;
    ctx.preds = malloc(ec_bst_size(score_tree) * sizeof *ctx.preds);
    if (!ctx.preds) {
        return EC_ENOMEM;
    }

    (void) ec_bst_walk(score_tree, &finalize_all_cb, &ctx);
    count = ctx.i;

    if (!*predict_cls || count > *n) {
        void *tmp;
        tmp = realloc(*predict_cls, count * sizeof **predict_cls);
        if (!tmp) {
            res = EC_ENOMEM;
            goto error;
        }
        *predict_cls = tmp;

        tmp = realloc(*predict_scores, count * sizeof **predict_scores);
        if (!tmp) {
            res = EC_ENOMEM;
            goto error;
        }
        *predict_scores = tmp;
    }
    *n = count;
    for (i = 0; i < count; i++) {
        (*predict_cls)[i] = ctx.preds[i].cls;
        (*predict_scores)[i] = ctx.preds[i].score;
    }

    res = EC_SUCCESS;
error:
    free(ctx.preds);
    return res;
}


struct max_cb_context {
    double score;
    ec_class cls;
};

static int
finalize_max_cb(union ec_bst_key key, union ec_bst_data data, void *opaque)
{
    ec_class cls = key.uint;
    double s = sc_finalize(data.ptr);
    struct max_cb_context *m = opaque;
    if (s > m->score) {
        m->score = s;
        m->cls = cls;
    }
    return EC_SUCCESS;
}

static int
finalize_max(struct ec_bst *scores, ec_class *predict_cls, double *predict_score)
{
    int res;
    struct max_cb_context m = { -INFINITY, -1 };
    res = ec_bst_walk(scores, finalize_max_cb, &m);
    if (!EC_ISERROR(res)) {
        *predict_cls = m.cls;
        *predict_score = m.score;
    }
    return res;
}

static int
scores_add(struct ec_bst *scores, ec_class cls, size_t index,
           double dist[static EC_SUFFIX_LEN], bool reverse)
{
    int res;
    union ec_bst_key key = { .uint = cls };
    union ec_bst_data data;
    res = ec_bst_get(scores, key, &data);
    if (res == EC_ENOENT) {
        if (!(data.ptr = sc_new())) {
            return EC_ENOMEM;
        }
        res = ec_bst_insert(scores, key, data);
    }
    if (EC_ISERROR(res)) {
        return res;
    }
    sc_add(data.ptr, index, dist, reverse);
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
    if (EC_ISERROR(res) || ec_word_equal(&lower_nb, &upper_nb)) {
        return res;
    }
    align_suffixes(dist, word->suffix, upper_nb.suffix, substmat);
    res = scores_add(scores, upper_cls, index, dist, reverse);
    return res;
}

static int
scores_compute(
        const char *seq,
        const struct ec_substmat substmat[static EC_SUFFIX_LEN],
        const struct ec_ecurve *fwd_ecurve,
        const struct ec_ecurve *rev_ecurve,
        struct ec_bst *scores)
{
    int res;
    struct ec_worditer iter;
    struct ec_alphabet alpha;
    size_t index;
    struct ec_word
        fwd_word = EC_WORD_INITIALIZER,
        rev_word = EC_WORD_INITIALIZER;

    ec_ecurve_get_alphabet(fwd_ecurve ? fwd_ecurve : rev_ecurve, &alpha);
    ec_worditer_init(&iter, seq, &alpha);

    while (EC_ITER_YIELD(res, ec_worditer_next(&iter, &index, &fwd_word, &rev_word))) {
        res = scores_add_word(scores, &fwd_word, index, false, fwd_ecurve, substmat);
        if (EC_ISERROR(res)) {
            break;
        }
        res = scores_add_word(scores, &rev_word, index, true, rev_ecurve, substmat);
        if (EC_ISERROR(res)) {
            break;
        }
    }
    if (!EC_ISERROR(res)) {
        res = EC_SUCCESS;
    }

    return res;
}

int
ec_classify_protein_all(
        const char *seq,
        const struct ec_substmat substmat[static EC_SUFFIX_LEN],
        const struct ec_ecurve *fwd_ecurve,
        const struct ec_ecurve *rev_ecurve,
        size_t *predict_count,
        ec_class **predict_cls,
        double **predict_score)
{
    int res;
    struct ec_bst scores;
    ec_bst_init(&scores, EC_BST_UINT);
    res = scores_compute(seq, substmat, fwd_ecurve, rev_ecurve, &scores);
    if (EC_ISERROR(res) || ec_bst_isempty(&scores)) {
        *predict_count = 0;
        goto error;
    }
    res = finalize_all(&scores, predict_count, predict_cls, predict_score);
error:
    ec_bst_clear(&scores, ec_bst_free_ptr);
    return res;
}

int
ec_classify_protein_max(
        const char *seq,
        const struct ec_substmat substmat[static EC_SUFFIX_LEN],
        const struct ec_ecurve *fwd_ecurve,
        const struct ec_ecurve *rev_ecurve,
        ec_class *predict_cls,
        double *predict_score)
{
    int res;
    struct ec_bst scores;
    ec_bst_init(&scores, EC_BST_UINT);
    res = scores_compute(seq, substmat, fwd_ecurve, rev_ecurve, &scores);
    if (EC_ISERROR(res) || ec_bst_isempty(&scores)) {
        goto error;
    }
    res = finalize_max(&scores, predict_cls, predict_score);
error:
    ec_bst_clear(&scores, ec_bst_free_ptr);
    return res;
}

int
ec_classify_dna_all(
        const char *seq,
        enum ec_orf_mode mode,
        const struct ec_orf_codonscores *codon_scores,
        const struct ec_matrix *thresholds,
        const struct ec_substmat substmat[static EC_SUFFIX_LEN],
        const struct ec_ecurve *fwd_ecurve,
        const struct ec_ecurve *rev_ecurve,
        size_t *predict_count,
        ec_class **predict_cls,
        double *predict_score[EC_ORF_FRAMES],
        size_t *orf_lengths)
{
    int res;
    unsigned i;
    char *orf[EC_ORF_FRAMES] = { NULL };
    size_t orf_sz[EC_ORF_FRAMES];

    res = ec_orf_chained(seq, mode, codon_scores, thresholds, orf, orf_sz);
    if (EC_ISERROR(res)) {
        goto error;
    }

    for (i = 0; i < mode; i++) {
        if (orf[i]) {
            res = ec_classify_protein_all(orf[i], substmat,
                    fwd_ecurve, rev_ecurve,
                    &predict_count[i], &predict_cls[i], &predict_score[i]);
            orf_lengths[i] = strlen(orf[i]);
            if (EC_ISERROR(res)) {
                goto error;
            }
        }
        else {
            predict_count[i] = 0;
            orf_lengths[i] = 0;
        }
    }

error:
    for (i = 0; i < mode; i++) {
        free(orf[i]);
    }
    return res;
}

int
ec_classify_dna_max(
        const char *seq,
        enum ec_orf_mode mode,
        const struct ec_orf_codonscores *codon_scores,
        const struct ec_matrix *thresholds,
        const struct ec_substmat substmat[static EC_SUFFIX_LEN],
        const struct ec_ecurve *fwd_ecurve,
        const struct ec_ecurve *rev_ecurve,
        ec_class *predict_cls,
        double *predict_score,
        size_t *orf_lengths)
{
    int res;
    unsigned i;
    char *orf[EC_ORF_FRAMES] = { NULL };
    size_t orf_sz[EC_ORF_FRAMES];

    res = ec_orf_chained(seq, mode, codon_scores, thresholds, orf, orf_sz);
    if (EC_ISERROR(res)) {
        goto error;
    }

    for (i = 0; i < mode; i++) {
        if (orf[i]) {
            res = ec_classify_protein_max(orf[i], substmat,
                    fwd_ecurve, rev_ecurve,
                    &predict_cls[i], &predict_score[i]);
            orf_lengths[i] = strlen(orf[i]);
            if (EC_ISERROR(res)) {
                goto error;
            }
        }
        else {
            predict_cls[i] = 0;
            predict_score[i] = -INFINITY;
            orf_lengths[i] = 0;
        }
    }

error:
    for (i = 0; i < mode; i++) {
        free(orf[i]);
    }
    return res;
}

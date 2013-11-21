#include <string.h>

#include "upro/common.h"
#include "upro/error.h"
#include "upro/bst.h"
#include "upro/protclass.h"



/*********************
 * score computation *
 *********************/

struct sc
{
    size_t index;
    double total, dist[UPRO_WORD_LEN];
};

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
sc_init(struct sc *s)
{
    size_t i;
    s->index = -1;
    s->total = 0.0;
    for (i = 0; i < UPRO_WORD_LEN; i++) {
        s->dist[i] = -INFINITY;
    }
}

static void
sc_add(struct sc *score, size_t index, double dist[static UPRO_SUFFIX_LEN],
       bool reverse)
{
    size_t i, diff;
    double tmp[UPRO_WORD_LEN];

    for (i = 0; i < UPRO_PREFIX_LEN; i++) {
        tmp[i] = -INFINITY;
    }
    memcpy(tmp + UPRO_PREFIX_LEN, dist, sizeof *dist * UPRO_SUFFIX_LEN);
    if (reverse) {
        reverse_array(tmp, UPRO_WORD_LEN, sizeof *tmp);
    }

    if (score->index != (size_t) -1) {
        diff = index - score->index;
        if (diff > UPRO_WORD_LEN) {
            diff = UPRO_WORD_LEN;
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

    for (i = 0; i + diff < UPRO_WORD_LEN; i++) {
#define MAX(a, b) (a > b ? a : b)
        score->dist[i] = MAX(score->dist[i + diff], tmp[i]);
    }
    for (; i < UPRO_WORD_LEN; i++) {
        score->dist[i] = tmp[i];
    }
    score->index = index;
}

static double
sc_finalize(struct sc *score)
{
    size_t i;
    for (i = 0; i < UPRO_WORD_LEN; i++) {
        if (isfinite(score->dist[i])) {
            score->total += score->dist[i];
        }
    }
    return score->total;
}

static int
scores_add(struct upro_bst *scores, upro_family family, size_t index,
           double dist[static UPRO_SUFFIX_LEN], bool reverse)
{
    int res;
    struct sc sc;
    union upro_bst_key key = { .uint = family };
    res = upro_bst_get(scores, key, &sc);
    if (res == UPRO_BST_KEY_NOT_FOUND) {
        sc_init(&sc);
    }
    sc_add(&sc, index, dist, reverse);
    return upro_bst_update(scores, key, &sc);
}

static void
align_suffixes(double dist[static UPRO_SUFFIX_LEN], upro_suffix s1, upro_suffix s2,
               const struct upro_substmat *substmat)
{
    size_t i;
    upro_amino a1, a2;
    for (i = 0; i < UPRO_SUFFIX_LEN; i++) {
        a1 = s1 & UPRO_BITMASK(UPRO_AMINO_BITS);
        a2 = s2 & UPRO_BITMASK(UPRO_AMINO_BITS);
        s1 >>= UPRO_AMINO_BITS;
        s2 >>= UPRO_AMINO_BITS;
        if (substmat) {
            dist[i] = upro_substmat_get(substmat, i, a1, a2);
        }
        else {
            dist[i] = a1 == a2 ? 1.0 : 0.0;
        }
    }
}

static int
scores_add_word(struct upro_bst *scores, const struct upro_word *word, size_t index,
                bool reverse, const struct upro_ecurve *ecurve,
                const struct upro_substmat *substmat)
{
    int res;
    struct upro_word
        lower_nb = UPRO_WORD_INITIALIZER,
        upper_nb = UPRO_WORD_INITIALIZER;
    upro_family lower_family, upper_family;
    double dist[UPRO_SUFFIX_LEN];

    if (!ecurve) {
        return UPRO_SUCCESS;
    }
    upro_ecurve_lookup(ecurve, word, &lower_nb, &lower_family, &upper_nb, &upper_family);
    align_suffixes(dist, word->suffix, lower_nb.suffix, substmat);
    res = scores_add(scores, lower_family, index, dist, reverse);
    if (res || upro_word_equal(&lower_nb, &upper_nb)) {
        return res;
    }
    align_suffixes(dist, word->suffix, upper_nb.suffix, substmat);
    res = scores_add(scores, upper_family, index, dist, reverse);
    return res;
}

static int
scores_compute(const struct upro_protclass *pc, const char *seq, struct upro_bst *scores)
{
    int res;
    struct upro_worditer iter;
    size_t index;
    struct upro_word
        fwd_word = UPRO_WORD_INITIALIZER,
        rev_word = UPRO_WORD_INITIALIZER;

    upro_worditer_init(&iter, seq,
        pc->fwd ? &pc->fwd->alphabet : &pc->rev->alphabet);

    while ((res = upro_worditer_next(&iter, &index, &fwd_word, &rev_word)) == UPRO_ITER_YIELD) {
        res = scores_add_word(scores, &fwd_word, index, false, pc->fwd,
                pc->substmat);
        if (res) {
            break;
        }
        res = scores_add_word(scores, &rev_word, index, true, pc->rev,
                pc->substmat);
        if (res) {
            break;
        }
    }
    if (res == UPRO_ITER_STOP) {
        res = UPRO_SUCCESS;
    }
    return res;
}


/****************
 * finalization *
 ****************/

static int
scores_finalize(const struct upro_protclass *pc, const char *seq,
        struct upro_bst *score_tree, struct upro_pc_results *results)
{
    int res = UPRO_SUCCESS;
    struct upro_bstiter iter;
    union upro_bst_key key;
    struct sc value;
    size_t seq_len = strlen(seq);

    results->n = upro_bst_size(score_tree);
    if (results->n > results->sz) {
        void *tmp;
        tmp = realloc(results->preds, results->n * sizeof *results->preds);
        if (!tmp) {
            res = upro_error(UPRO_ENOMEM);
            goto error;
        }
        results->preds = tmp;
        results->sz = results->n;
    }

    results->n = 0;
    upro_bstiter_init(&iter, score_tree);
    while (upro_bstiter_next(&iter, &key, &value) == UPRO_ITER_YIELD) {
        upro_family family = key.uint;
        double score = sc_finalize(&value);
        if (pc->filter &&
                !pc->filter(seq, seq_len, family, score, pc->filter_arg)) {
            continue;
        }
        results->preds[results->n].score = score;
        results->preds[results->n].family = family;
        results->n++;
    }

    if (pc->mode == UPRO_PC_MAX && results->n > 0) {
        size_t imax = 0;
        for (size_t i = 1; i < results->n; i++) {
            if (results->preds[i].score > results->preds[imax].score) {
                imax = i;
            }
        }
        results->preds[0] = results->preds[imax];
        results->n = 1;
    }
    res = UPRO_SUCCESS;
error:
    return res;
}


/**********************
 * exported functions *
 **********************/

int
upro_pc_init(struct upro_protclass *pc,
        enum upro_pc_mode mode,
        const struct upro_ecurve *fwd,
        const struct upro_ecurve *rev,
        const struct upro_substmat *substmat,
        upro_pc_filter *filter, void *filter_arg)
{
    if (!(fwd || rev)) {
        return upro_error_msg(
            UPRO_EINVAL, "protein classifier requires at least one ecurve");
    }
    *pc = (struct upro_protclass) {
        .mode = mode,
        .substmat = substmat,
        .fwd = fwd,
        .rev = rev,
        .filter = filter,
        .filter_arg = filter_arg,
    };
    return UPRO_SUCCESS;
}

static int
classify(const struct upro_protclass *pc, const char *seq,
        struct upro_pc_results *results)
{
    int res;
    struct upro_bst scores;

    upro_bst_init(&scores, UPRO_BST_UINT, sizeof (struct sc));
    res = scores_compute(pc, seq, &scores);
    if (res || upro_bst_isempty(&scores)) {
        results->n = 0;
        goto error;
    }
    res = scores_finalize(pc, seq, &scores, results);
error:
    upro_bst_clear(&scores, NULL);
    return res;
}

int
upro_pc_classify(const struct upro_protclass *pc, const char *seq,
        struct upro_pc_results *results)
{
    int res;
    size_t i, imax = 0;
    res = classify(pc, seq, results);
    if (res || !results->n) {
        return res;
    }
    if (pc->mode == UPRO_PC_MAX) {
        for (i = 1; i < results->n; i++) {
            if (results->preds[i].score > results->preds[imax].score) {
                imax = i;
            }
        }
        results->preds[0] = results->preds[imax];
        results->n = 1;
    }
    return UPRO_SUCCESS;
}

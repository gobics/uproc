#include <string.h>

#include "ecurve/protclass.h"
#include "ecurve/bst.h"



/*********************
 * score computation *
 *********************/

struct sc
{
    size_t index;
    double total, dist[EC_WORD_LEN];
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
    for (i = 0; i < EC_WORD_LEN; i++) {
        s->dist[i] = -INFINITY;
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

static int
scores_add(struct ec_bst *scores, ec_class cls, size_t index,
           double dist[static EC_SUFFIX_LEN], bool reverse)
{
    int res;
    struct sc sc;
    union ec_bst_key key = { .uint = cls };
    res = ec_bst_get(scores, key, &sc);
    if (res == EC_ENOENT) {
        sc_init(&sc);
    }
    sc_add(&sc, index, dist, reverse);
    res = ec_bst_update(scores, key, &sc);
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
scores_compute(const struct ec_protclass *pc, const char *seq, struct ec_bst *scores)
{
    int res;
    struct ec_worditer iter;
    size_t index;
    struct ec_word
        fwd_word = EC_WORD_INITIALIZER,
        rev_word = EC_WORD_INITIALIZER;

    ec_worditer_init(&iter, seq,
        pc->fwd ? &pc->fwd->alphabet : &pc->rev->alphabet);

    while ((res = ec_worditer_next(&iter, &index, &fwd_word, &rev_word)) == EC_ITER_YIELD) {
        res = scores_add_word(scores, &fwd_word, index, false, pc->fwd,
                pc->substmat);
        if (EC_ISERROR(res)) {
            break;
        }
        res = scores_add_word(scores, &rev_word, index, true, pc->rev,
                pc->substmat);
        if (EC_ISERROR(res)) {
            break;
        }
    }
    if (!EC_ISERROR(res)) {
        res = EC_SUCCESS;
    }
    return res;
}


/****************
 * finalization *
 ****************/

static int
scores_finalize(const struct ec_protclass *pc, const char *seq,
        struct ec_bst *score_tree, struct ec_pc_results *results)
{
    int res = EC_SUCCESS;
    struct ec_bstiter iter;
    union ec_bst_key key;
    struct sc value;
    size_t seq_len = strlen(seq);

    results->n = ec_bst_size(score_tree);
    if (results->n > results->sz) {
        void *tmp;
        tmp = realloc(results->preds, results->n * sizeof *results->preds);
        if (!tmp) {
            res = EC_ENOMEM;
            goto error;
        }
        results->preds = tmp;
        results->sz = results->n;
    }

    results->n = 0;
    ec_bstiter_init(&iter, score_tree);
    while (ec_bstiter_next(&iter, &key, &value) == EC_ITER_YIELD) {
        ec_class cls = key.uint;
        double score = sc_finalize(&value);
        if (pc->filter &&
                !pc->filter(seq, seq_len, cls, score, pc->filter_arg)) {
            continue;
        }
        results->preds[results->n].score = score;
        results->preds[results->n].cls = cls;
        results->n++;
    }

    if (pc->mode == EC_PC_MAX && results->n > 0) {
        size_t imax = 0;
        for (size_t i = 1; i < results->n; i++) {
            if (results->preds[i].score > results->preds[imax].score) {
                imax = i;
            }
        }
        results->preds[0] = results->preds[imax];
        results->n = 1;
    }
    res = EC_SUCCESS;
error:
    return res;
}


/**********************
 * exported functions *
 **********************/

int
ec_pc_init(struct ec_protclass *pc,
        enum ec_pc_mode mode,
        const struct ec_ecurve *fwd,
        const struct ec_ecurve *rev,
        const struct ec_substmat *substmat,
        ec_pc_filter *filter, void *filter_arg)
{
    if (!(fwd || rev)) {
        return EC_EINVAL;
    }
    *pc = (struct ec_protclass) {
        .mode = mode,
        .substmat = substmat,
        .fwd = fwd,
        .rev = rev,
        .filter = filter,
        .filter_arg = filter_arg,
    };
    return EC_SUCCESS;
}

static int
classify(const struct ec_protclass *pc, const char *seq,
        struct ec_pc_results *results)
{
    int res;
    struct ec_bst scores;

    ec_bst_init(&scores, EC_BST_UINT, sizeof (struct sc));
    res = scores_compute(pc, seq, &scores);
    if (EC_ISERROR(res) || ec_bst_isempty(&scores)) {
        results->n = 0;
        goto error;
    }
    res = scores_finalize(pc, seq, &scores, results);
error:
    ec_bst_clear(&scores, NULL);
    return res;
}

int
ec_pc_classify(const struct ec_protclass *pc, const char *seq,
        struct ec_pc_results *results)
{
    int res;
    size_t i, imax = 0;
    res = classify(pc, seq, results);
    if (EC_ISERROR(res) || !results->n) {
        return res;
    }
    if (pc->mode == EC_PC_MAX) {
        for (i = 1; i < results->n; i++) {
            if (results->preds[i].score > results->preds[imax].score) {
                imax = i;
            }
        }
        results->preds[0] = results->preds[imax];
        results->n = 1;
    }
    return EC_SUCCESS;
}

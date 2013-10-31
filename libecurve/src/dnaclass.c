#include <stdlib.h>
#include <ctype.h>

#include "ecurve/common.h"
#include "ecurve/bst.h"
#include "ecurve/dnaclass.h"
#include "ecurve/protclass.h"
#include "ecurve/orf.h"

int
ec_dc_init(struct ec_dnaclass *dc,
        enum ec_dc_mode mode,
        const struct ec_protclass *pc,
        const struct ec_orf_codonscores *codon_scores,
        ec_orf_filter *orf_filter, void *orf_filter_arg)
{
    if (!pc) {
        return EC_EINVAL;
    }
    *dc = (struct ec_dnaclass) {
        .mode = mode,
        .pc = pc,
        .codon_scores = codon_scores,
        .orf_filter = orf_filter,
        .orf_filter_arg = orf_filter_arg,
    };
    return EC_SUCCESS;
}

int
ec_dc_classify(const struct ec_dnaclass *dc, const char *seq, struct ec_dc_results *results)
{
    int res;
    size_t i;
    struct ec_orf orf;
    struct ec_orfiter orf_iter;
    struct ec_bst max_scores;
    struct ec_bstiter max_scores_iter;
    union ec_bst_key key;
    struct { double score; unsigned frame; } value;
    struct ec_pc_results pc_res = { NULL, 0, 0 };

    ec_bst_init(&max_scores, EC_BST_UINT, sizeof value);

    res = ec_orfiter_init(&orf_iter, seq, dc->codon_scores,
            dc->orf_filter, dc->orf_filter_arg);
    if (EC_ISERROR(res)) {
        return res;
    }

    while ((res = ec_orfiter_next(&orf_iter, &orf)) == EC_ITER_YIELD) {
        res = ec_pc_classify(dc->pc, orf.data, &pc_res);
        if (EC_ISERROR(res)) {
            goto error;
        }
        for (i = 0; i < pc_res.n; i++) {
            double score = pc_res.preds[i].score;
            key.uint = pc_res.preds[i].cls;
            value.score = -INFINITY;
            (void) ec_bst_get(&max_scores, key, &value);
            if (score > value.score) {
                value.score = score;
                value.frame = orf.frame;
                res = ec_bst_update(&max_scores, key, &value);
                if (EC_ISERROR(res)) {
                    goto error;
                }
            }
        }
    }

    results->n = ec_bst_size(&max_scores);
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
    ec_bstiter_init(&max_scores_iter, &max_scores);
    for (i = 0; ec_bstiter_next(&max_scores_iter, &key, &value) == EC_ITER_YIELD; i++) {
        results->preds[i].cls = key.uint;
        results->preds[i].score = value.score;
        results->preds[i].frame = value.frame;
    }

    if (dc->mode == EC_DC_MAX && results->n > 0) {
        size_t imax = 0;
        for (i = 1; i < results->n; i++) {
            if (results->preds[i].score > results->preds[imax].score) {
                imax = i;
            }
        }
        results->preds[0] = results->preds[imax];
        results->n = 1;
    }

    if (0) {
error:
        results->n = 0;
    }
    free(pc_res.preds);
    ec_bst_clear(&max_scores, NULL);
    ec_orfiter_destroy(&orf_iter);
    return res;
}

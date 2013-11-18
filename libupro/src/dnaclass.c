#include <stdlib.h>
#include <ctype.h>

#include "upro/common.h"
#include "upro/error.h"
#include "upro/bst.h"
#include "upro/dnaclass.h"
#include "upro/protclass.h"
#include "upro/orf.h"

int
upro_dc_init(struct upro_dnaclass *dc,
        enum upro_dc_mode mode,
        const struct upro_protclass *pc,
        const struct upro_matrix *codon_scores,
        upro_orf_filter *orf_filter, void *orf_filter_arg)
{
    if (!pc) {
        return upro_error_msg(UPRO_EINVAL,
                              "DNA classifier requires a protein classifier");
    }
    *dc = (struct upro_dnaclass) {
        .mode = mode,
        .pc = pc,
        .orf_filter = orf_filter,
        .orf_filter_arg = orf_filter_arg,
    };
    upro_orf_codonscores(dc->codon_scores, codon_scores);
    return UPRO_SUCCESS;
}

int
upro_dc_classify(const struct upro_dnaclass *dc, const char *seq, struct upro_dc_results *results)
{
    int res;
    size_t i;
    struct upro_orf orf;
    struct upro_orfiter orf_iter;
    struct upro_bst max_scores;
    struct upro_bstiter max_scores_iter;
    union upro_bst_key key;
    struct { double score; unsigned frame; } value;
    struct upro_pc_results pc_res = { NULL, 0, 0 };

    upro_bst_init(&max_scores, UPRO_BST_UINT, sizeof value);

    res = upro_orfiter_init(&orf_iter, seq, dc->codon_scores,
            dc->orf_filter, dc->orf_filter_arg);
    if (res) {
        return res;
    }

    while ((res = upro_orfiter_next(&orf_iter, &orf)) == UPRO_ITER_YIELD) {
        res = upro_pc_classify(dc->pc, orf.data, &pc_res);
        if (res) {
            goto error;
        }
        for (i = 0; i < pc_res.n; i++) {
            double score = pc_res.preds[i].score;
            key.uint = pc_res.preds[i].family;
            value.score = -INFINITY;
            (void) upro_bst_get(&max_scores, key, &value);
            if (score > value.score) {
                value.score = score;
                value.frame = orf.frame;
                res = upro_bst_update(&max_scores, key, &value);
                if (res) {
                    goto error;
                }
            }
        }
    }

    results->n = upro_bst_size(&max_scores);
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
    upro_bstiter_init(&max_scores_iter, &max_scores);
    for (i = 0; upro_bstiter_next(&max_scores_iter, &key, &value) == UPRO_ITER_YIELD; i++) {
        results->preds[i].family = key.uint;
        results->preds[i].score = value.score;
        results->preds[i].frame = value.frame;
    }

    if (dc->mode == UPRO_DC_MAX && results->n > 0) {
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
    upro_bst_clear(&max_scores, NULL);
    upro_orfiter_destroy(&orf_iter);
    return res;
}

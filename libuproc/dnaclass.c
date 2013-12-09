/* Classify DNA/RNA sequences
 *
 * Copyright 2013 Peter Meinicke, Robin Martinjak
 *
 * This file is part of libuproc.
 *
 * libuproc is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libuproc is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libuproc.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/bst.h"
#include "uproc/dnaclass.h"
#include "uproc/protclass.h"
#include "uproc/orf.h"

struct uproc_dnaclass_s
{
    enum uproc_dc_mode mode;
    const uproc_protclass *pc;
    double codon_scores[UPROC_BINARY_CODON_COUNT];
    uproc_orf_filter *orf_filter;
    void *orf_filter_arg;
};

uproc_dnaclass *
uproc_dc_create(enum uproc_dc_mode mode, const uproc_protclass *pc,
                const uproc_matrix *codon_scores, uproc_orf_filter *orf_filter,
                void *orf_filter_arg)
{
    struct uproc_dnaclass_s *dc;
    if (!pc) {
        uproc_error_msg(UPROC_EINVAL,
                        "DNA classifier requires a protein classifier");
        return NULL;
    }
    dc = malloc(sizeof *dc);
    if (!dc) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }

    *dc = (struct uproc_dnaclass_s) {
        .mode = mode,
        .pc = pc,
        .orf_filter = orf_filter,
        .orf_filter_arg = orf_filter_arg,
    };
    uproc_orf_codonscores(dc->codon_scores, codon_scores);
    return dc;
}

void
uproc_dc_destroy(uproc_dnaclass *dc)
{
    free(dc);
}

int
uproc_dc_classify(const uproc_dnaclass *dc, const char *seq,
                  struct uproc_dc_results *results)
{
    int res;
    size_t i;
    struct uproc_orf orf;
    uproc_orfiter *orf_iter;
    uproc_bst *max_scores;
    uproc_bstiter *max_scores_iter;
    union uproc_bst_key key;
    struct { double score; unsigned frame; } value;
    struct uproc_pc_results pc_res = { NULL, 0, 0 };

    max_scores = uproc_bst_create(UPROC_BST_UINT, sizeof value);
    if (!max_scores) {
        return -1;
    }

    orf_iter = uproc_orfiter_create(seq, dc->codon_scores, dc->orf_filter,
                                    dc->orf_filter_arg);
    if (!orf_iter) {
        return -1;
    }

    while ((res = uproc_orfiter_next(orf_iter, &orf)) > 0) {
        res = uproc_pc_classify(dc->pc, orf.data, &pc_res);
        if (res) {
            goto error;
        }
        for (i = 0; i < pc_res.n; i++) {
            double score = pc_res.preds[i].score;
            key.uint = pc_res.preds[i].family;
            value.score = -INFINITY;
            (void) uproc_bst_get(max_scores, key, &value);
            if (score > value.score) {
                value.score = score;
                value.frame = orf.frame;
                res = uproc_bst_update(max_scores, key, &value);
                if (res) {
                    goto error;
                }
            }
        }
    }

    results->n = uproc_bst_size(max_scores);
    if (results->n > results->sz) {
        void *tmp;
        tmp = realloc(results->preds, results->n * sizeof *results->preds);
        if (!tmp) {
            res = uproc_error(UPROC_ENOMEM);
            goto error;
        }
        results->preds = tmp;
        results->sz = results->n;
    }
    max_scores_iter = uproc_bstiter_create(max_scores);
    if (!max_scores_iter) {
        goto error;
    }
    for (i = 0; uproc_bstiter_next(max_scores_iter, &key, &value) > 0; i++) {
        results->preds[i].family = key.uint;
        results->preds[i].score = value.score;
        results->preds[i].frame = value.frame;
    }
    uproc_bstiter_destroy(max_scores_iter);

    if (dc->mode == UPROC_DC_MAX && results->n > 0) {
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
    uproc_bst_destroy(max_scores);
    uproc_orfiter_destroy(orf_iter);
    return res;
}

/* Classify DNA/RNA sequences
 *
 * Copyright 2014 Peter Meinicke, Robin Martinjak
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
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "uproc/common.h"
#include "uproc/codon.h"
#include "uproc/error.h"
#include "uproc/bst.h"
#include "uproc/list.h"
#include "uproc/dnaclass.h"
#include "uproc/protclass.h"
#include "uproc/orf.h"

struct uproc_dnaclass_s
{
    enum uproc_dnaclass_mode mode;
    const uproc_protclass *pc;
    double codon_scores[UPROC_BINARY_CODON_COUNT];
    uproc_orffilter *orf_filter;
    void *orf_filter_arg;
};

uproc_dnaclass *
uproc_dnaclass_create(enum uproc_dnaclass_mode mode, const uproc_protclass *pc,
                      const uproc_matrix *codon_scores, uproc_orffilter *orf_filter,
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
uproc_dnaclass_destroy(uproc_dnaclass *dc)
{
    free(dc);
}


static void
map_list_dnaresult_free(void *value, void *opaque)
{
    (void) opaque;
    uproc_dnaresult_free(value);
}


static void
map_bst_dnaresult_free(union uproc_bst_key key, void *value, void *opaque)
{
    (void) key;
    (void) opaque;
    uproc_dnaresult_free(value);
}


int
uproc_dnaclass_classify(const uproc_dnaclass *dc, const char *seq,
                        uproc_list **results)
{
    int res;
    size_t i;
    struct uproc_orf orf;
    uproc_orfiter *orf_iter;
    uproc_bst *max_scores;
    uproc_bstiter *max_scores_iter;
    union uproc_bst_key key;
    uproc_list *pc_results = NULL;

    struct uproc_dnaresult
        pred = UPROC_DNARESULT_INITIALIZER,
        pred_max = UPROC_DNARESULT_INITIALIZER;

    if (!*results) {
        *results = uproc_list_create(sizeof pred);
        if (!*results) {
            return -1;
        }
    }
    else {
        uproc_list_map(*results, map_list_dnaresult_free, NULL);
        uproc_list_clear(*results);
    }

    max_scores = uproc_bst_create(UPROC_BST_UINT, sizeof pred);
    orf_iter = uproc_orfiter_create(seq, dc->codon_scores, dc->orf_filter,
                                    dc->orf_filter_arg);

    if (!max_scores || !orf_iter) {
        res = -1;
        goto error;
    }

    while (res = uproc_orfiter_next(orf_iter, &orf), !res) {
        res = uproc_protclass_classify(dc->pc, orf.data, &pc_results);
        if (res) {
            goto error;
        }
        for (size_t n = uproc_list_size(pc_results), i = 0; i < n; i++) {
            struct uproc_protresult pp;
            (void) uproc_list_get(pc_results, i, &pp);
            key.uint = pp.family;
            uproc_dnaresult_init(&pred);
            pred.score = -INFINITY;
            (void) uproc_bst_get(max_scores, key, &pred);
            if (pp.score > pred.score) {
                uproc_dnaresult_free(&pred);
                pred.family = pp.family;
                pred.score = pp.score;
                res = uproc_orf_copy(&pred.orf, &orf);
                if (res) {
                    goto error;
                }
                res = uproc_bst_update(max_scores, key, &pred);
                if (res) {
                    goto error;
                }
            }
        }
    }
    if (res == -1) {
        goto error;
    }

    max_scores_iter = uproc_bstiter_create(max_scores);
    if (!max_scores_iter) {
        goto error;
    }
    for (i = 0; !uproc_bstiter_next(max_scores_iter, &key, &pred); i++) {
        if (dc->mode == UPROC_DNACLASS_MAX) {
            if (!uproc_list_size(*results)) {
                pred_max = pred;
                res = uproc_list_append(*results, &pred);
                if (res) {
                    break;
                }
            }
            else if (pred.score > pred_max.score) {
                uproc_dnaresult_free(&pred_max);
                pred_max = pred;
                uproc_list_set(*results, 0, &pred_max);
            }
            else {
                uproc_dnaresult_free(&pred);
            }
        }
        else {
            res = uproc_list_append(*results, &pred);
            if (res) {
                goto error;
            }
        }
    }
    uproc_bstiter_destroy(max_scores_iter);

    res = 0;

    if (0) {
error:
        uproc_bst_map(max_scores, map_bst_dnaresult_free, NULL);
    }
    uproc_list_destroy(pc_results);
    uproc_bst_destroy(max_scores);
    uproc_orfiter_destroy(orf_iter);
    return res;
}

void
uproc_dnaresult_init(struct uproc_dnaresult *result)
{
    *result = (struct uproc_dnaresult) UPROC_DNARESULT_INITIALIZER;
}

void
uproc_dnaresult_free(struct uproc_dnaresult *result)
{
    uproc_orf_free(&result->orf);
}

int
uproc_dnaresult_copy(struct uproc_dnaresult *dest,
                      const struct uproc_dnaresult *src)
{
    *dest = *src;
    return uproc_orf_copy(&dest->orf, &src->orf);
}

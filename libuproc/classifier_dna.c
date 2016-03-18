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
#include "uproc/dict.h"
#include "uproc/list.h"
#include "uproc/classifier.h"
#include "uproc/orf.h"

#include "classifier_internal.h"

struct context {
    uproc_clf *protclass;
    double codon_scores[UPROC_BINARY_CODON_COUNT];
    uproc_orffilter *orf_filter;
    void *orf_filter_arg;
};

static void map_dict_result_free(void *key, void *value, void *opaque)
{
    (void)key;
    (void)opaque;
    // uproc_result_free() works on shallow copies as well (as the pointer
    // itself is not free()'d).
    uproc_result_free(value);
}

static int classify(const void *context, const char *seq, uproc_list *results)
{
    uproc_assert(context);
    uproc_assert(seq);
    uproc_assert(results);

    const struct context *ctx = context;

    int res;

    // max_scores maps  (rank, class) -> result with the max score
    struct {
        uproc_rank rank;
        uproc_class class;
    } max_scores_key;
    memset(&max_scores_key, 0, sizeof max_scores_key);
    uproc_dict *max_scores =
        uproc_dict_create(sizeof max_scores_key, sizeof(struct uproc_result));
    if (!max_scores) {
        return -1;
    }

    uproc_orfiter *orf_iter = uproc_orfiter_create(
        seq, ctx->codon_scores, ctx->orf_filter, ctx->orf_filter_arg);

    if (!orf_iter) {
        uproc_dict_destroy(max_scores);
        return -1;
    }

    uproc_list *pc_results = NULL;
    struct uproc_orf orf;
    while (res = uproc_orfiter_next(orf_iter, &orf), !res) {
        res = uproc_clf_classify(ctx->protclass, orf.data, &pc_results);
        if (res) {
            goto error;
        }
        for (long i = 0, n = uproc_list_size(pc_results); i < n; i++) {
            struct uproc_result protresult;
            (void)uproc_list_get(pc_results, i, &protresult);

            struct uproc_result result = UPROC_RESULT_INITIALIZER;
            result.score = -INFINITY;

            max_scores_key.rank = protresult.rank;
            max_scores_key.class = protresult.class;
            (void)uproc_dict_get(max_scores, &max_scores_key, &result);
            if (protresult.score > result.score) {
                // free the old maximum value
                uproc_result_free(&result);

                // copy data for new value
                res = uproc_result_copy(&result, &protresult);
                if (res) {
                    goto error;
                }
                res = uproc_orf_copy(&result.orf, &orf);
                if (res) {
                    goto error;
                }

                // store new value for key
                res = uproc_dict_set(max_scores, &max_scores_key, &result);
                if (res) {
                    goto error;
                }
            }
        }
    }
    if (res == -1) {
        goto error;
    }

    uproc_dictiter *max_scores_iter = uproc_dictiter_create(max_scores);
    if (!max_scores_iter) {
        goto error;
    }

    struct uproc_result result;
    while (res = uproc_dictiter_next(max_scores_iter, &max_scores_key, &result),
           !res) {
        uproc_list_append(results, &result);
    }
    uproc_dictiter_destroy(max_scores_iter);
    if (res == -1) {
        goto error;
    }
    res = 0;

    if (0) {
    error:
        uproc_dict_map(max_scores, map_dict_result_free, NULL);
    }
    uproc_list_destroy(pc_results);
    uproc_dict_destroy(max_scores);
    uproc_orfiter_destroy(orf_iter);
    return res;
}

static void destroy(void *context)
{
    struct context *ctx = context;
    uproc_clf_destroy(ctx->protclass);
    free(ctx);
}

uproc_clf *uproc_clf_create_dna(enum uproc_clf_mode mode, uproc_clf *protclass,
                                const uproc_matrix *codon_scores,
                                uproc_orffilter *orf_filter,
                                void *orf_filter_arg)
{
    struct context *ctx;
    if (!protclass) {
        uproc_error_msg(UPROC_EINVAL,
                        "DNA classifier requires a protein classifier");
        return NULL;
    }
    ctx = malloc(sizeof *ctx);
    if (!ctx) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }

    *ctx = (struct context){
        .protclass = protclass,
        .orf_filter = orf_filter,
        .orf_filter_arg = orf_filter_arg,
    };
    uproc_orf_codonscores(ctx->codon_scores, codon_scores);

    return uproc_clf_create(mode, ctx, classify, destroy);
}

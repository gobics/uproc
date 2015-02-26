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

uproc_dnaclass *uproc_dnaclass_create(enum uproc_dnaclass_mode mode,
                                      const uproc_protclass *pc,
                                      const uproc_matrix *codon_scores,
                                      uproc_orffilter *orf_filter,
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

    *dc = (struct uproc_dnaclass_s){
        .mode = mode,
        .pc = pc,
        .orf_filter = orf_filter,
        .orf_filter_arg = orf_filter_arg,
    };
    uproc_orf_codonscores(dc->codon_scores, codon_scores);
    return dc;
}

void uproc_dnaclass_destroy(uproc_dnaclass *dc)
{
    free(dc);
}

static void map_list_dnaresult_free(void *value, void *opaque)
{
    (void)opaque;
    uproc_dnaresult_free(value);
}

static void map_dict_dnaresult_free(void *key, void *value, void *opaque)
{
    (void)key;
    (void)opaque;
    // uproc_dnaresult_free() works on shallow copies as well (as the pointer
    // itself is not free()'d).
    uproc_dnaresult_free(value);
}

// If value is greater than opaque (lower rank OR same rank with higher
// score), overwrite opaque with value. Frees the smaller value.
static void map_list_dnaresult_max(void *value, void *opaque)
{
    struct uproc_dnaresult *v = value, *max = opaque;
    if (uproc_protresult_cmp(&v->protresult, &max->protresult) > 0) {
        uproc_dnaresult_free(max);
        uproc_dnaresult_copy(max, v);
    } else {
        uproc_dnaresult_free(v);
    }
}

int uproc_dnaclass_classify(const uproc_dnaclass *dc, const char *seq,
                            uproc_list **results)
{
    int res;

    if (!*results) {
        *results = uproc_list_create(sizeof(struct uproc_dnaresult));
        if (!*results) {
            return -1;
        }
    } else {
        uproc_list_map(*results, map_list_dnaresult_free, NULL);
        uproc_list_clear(*results);
    }

    // max_scores maps  (rank, class) -> dnaresult with the max score
    struct
    {
        uproc_rank rank;
        uproc_class class;
    } max_scores_key;
    memset(&max_scores_key, 0, sizeof max_scores_key);
    uproc_dict *max_scores = uproc_dict_create(sizeof max_scores_key,
                                               sizeof(struct uproc_dnaresult));
    if (!max_scores) {
        return -1;
    }

    uproc_orfiter *orf_iter = uproc_orfiter_create(
        seq, dc->codon_scores, dc->orf_filter, dc->orf_filter_arg);

    if (!max_scores || !orf_iter) {
        res = -1;
        goto error;
    }

    uproc_list *pc_results = NULL;
    struct uproc_orf orf;
    while (res = uproc_orfiter_next(orf_iter, &orf), !res) {
        res = uproc_protclass_classify(dc->pc, orf.data, &pc_results);
        if (res) {
            goto error;
        }
        for (long i = 0, n = uproc_list_size(pc_results); i < n; i++) {
            struct uproc_protresult protresult;
            (void)uproc_list_get(pc_results, i, &protresult);

            struct uproc_dnaresult result = UPROC_DNARESULT_INITIALIZER;
            result.protresult.score = -INFINITY;

            max_scores_key.rank = protresult.rank;
            max_scores_key.class = protresult.class;
            (void)uproc_dict_get(max_scores, &max_scores_key, &result);
            if (protresult.score > result.protresult.score) {
                // free the old maximum value
                uproc_dnaresult_free(&result);

                // copy data for new value
                res = uproc_protresult_copy(&result.protresult, &protresult);
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

    struct uproc_dnaresult result;
    while (res = uproc_dictiter_next(max_scores_iter, &max_scores_key, &result),
           !res) {
        uproc_list_append(*results, &result);
    }
    uproc_dictiter_destroy(max_scores_iter);
    if (res == -1) {
        goto error;
    }
    res = 0;

    if (dc->mode == UPROC_DNACLASS_MAX) {
        // Find the classification result on the
        // lowest rank, and there with the highest score
        // then throw anything else in the list away.
        struct uproc_dnaresult result_max = UPROC_DNARESULT_INITIALIZER;
        result_max.protresult.rank = UPROC_RANKS_MAX;
        result_max.protresult.score = -INFINITY;
        // takes care of freeing list elements.
        uproc_list_map(*results, map_list_dnaresult_max, &result_max);
        uproc_list_clear(*results);
        res = uproc_list_append(*results, &result_max);
        if (res) {
            goto error;
        }
    }

    if (0) {
    error:
        uproc_dict_map(max_scores, map_dict_dnaresult_free, NULL);
    }
    uproc_list_destroy(pc_results);
    uproc_dict_destroy(max_scores);
    uproc_orfiter_destroy(orf_iter);
    return res;
}

void uproc_dnaresult_init(struct uproc_dnaresult *result)
{
    *result = (struct uproc_dnaresult)UPROC_DNARESULT_INITIALIZER;
}

void uproc_dnaresult_free(struct uproc_dnaresult *result)
{
    uproc_orf_free(&result->orf);
    uproc_protresult_free(&result->protresult);
}

int uproc_dnaresult_copy(struct uproc_dnaresult *dest,
                         const struct uproc_dnaresult *src)
{
    *dest = *src;
    return uproc_orf_copy(&dest->orf, &src->orf);
}

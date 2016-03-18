/* Classify protein sequences
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

#include <stdbool.h>
#include <string.h>

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/bst.h"
#include "uproc/list.h"
#include "uproc/mosaic.h"
#include "uproc/classifier.h"

#include "classifier_internal.h"
#include "ecurve_internal.h"

struct context {
    bool detailed;
    const uproc_substmat *substmat;
    const uproc_ecurve *fwd, *rev;
    uproc_protfilter *filter;
    void *filter_arg;
};

static inline uproc_rank get_ranks_count(const struct context *ctx)
{
    if (!ctx->fwd) {
        return ctx->rev->ranks_count;
    }
    return ctx->fwd->ranks_count;
}

static int scores_add(uproc_bst *scores, const struct uproc_word *word,
                      uproc_class class, size_t index,
                      double dist[static UPROC_SUFFIX_LEN], bool reverse,
                      bool detailed)
{
    if (class == UPROC_CLASS_INVALID) {
        return 0;
    }
    union uproc_bst_key key = {.uint = class};
    uproc_mosaic *m;
    int res = uproc_bst_get(scores, key, &m);
    if (res == UPROC_BST_KEY_NOT_FOUND) {
        m = uproc_mosaic_create(detailed);
        if (!m) {
            return -1;
        }
        res = uproc_bst_update(scores, key, &m);
        if (res) {
            return -1;
        }
    }
    return uproc_mosaic_add(m, word, index, dist, reverse);
}

static int scores_add_word(const struct context *ctx,
                           uproc_bst *scores[UPROC_RANKS_MAX],
                           const struct uproc_word *word, size_t index,
                           bool reverse, const uproc_ecurve *ecurve,
                           const uproc_substmat *substmat)
{
    int res = 0;
    struct uproc_word nb_words[2];
    uproc_class nb_classes[2][UPROC_RANKS_MAX];
    double dist[UPROC_SUFFIX_LEN];

    if (!ecurve) {
        return 0;
    }
    uproc_ecurve_lookup(ecurve, word, &nb_words[0], nb_classes[0], &nb_words[1],
                        nb_classes[1]);

    int ranks_count = get_ranks_count(ctx);
    for (int i = ranks_count; i < UPROC_RANKS_MAX; i++) {
        nb_classes[0][i] = nb_classes[1][i] = UPROC_CLASS_INVALID;
    }

    // Iff both words are equal, we start at index 1 instead of 0
    for (int i = !uproc_word_cmp(&nb_words[0], &nb_words[1]); !res && i < 2;
         i++) {
        uproc_substmat_align_suffixes(substmat, word->suffix,
                                      nb_words[i].suffix, dist);
        for (int k = 0; !res && k < ranks_count; k++) {
            uproc_assert(scores[k]);
            res = scores_add(scores[k], &nb_words[i], nb_classes[i][k], index,
                             dist, reverse, ctx->detailed);
        }
    }
    return res;
}

static int scores_compute(const struct context *ctx, const char *seq,
                          uproc_bst *scores[UPROC_RANKS_MAX])
{
    int res;
    uproc_worditer *iter;
    size_t index;
    struct uproc_word fwd_word = UPROC_WORD_INITIALIZER,
                      rev_word = UPROC_WORD_INITIALIZER;

    iter = uproc_worditer_create(seq, uproc_ecurve_alphabet(ctx->fwd));
    if (!iter) {
        return -1;
    }

    while (res = uproc_worditer_next(iter, &index, &fwd_word, &rev_word),
           !res) {
        res = scores_add_word(ctx, scores, &fwd_word, index, false, ctx->fwd,
                              ctx->substmat);
        if (res) {
            break;
        }
        res = scores_add_word(ctx, scores, &rev_word, index, true, ctx->rev,
                              ctx->substmat);
        if (res) {
            break;
        }
    }
    uproc_worditer_destroy(iter);
    return res == -1 ? -1 : 0;
}

/****************
 * finalization *
 ****************/

static int scores_finalize(const struct context *ctx, const char *seq,
                           uproc_bst *scores[UPROC_RANKS_MAX],
                           uproc_list *results)
{
    int res = 0;
    size_t seq_len = strlen(seq);

    for (int i = 0, n = get_ranks_count(ctx); i < n; i++) {
        if (!scores[i] || !uproc_bst_size(scores[i])) {
            continue;
        }

        uproc_bstiter *iter = uproc_bstiter_create(scores[i]);
        if (!iter) {
            return -1;
        }
        union uproc_bst_key key;
        uproc_mosaic *mosaic;
        while (!uproc_bstiter_next(iter, &key, &mosaic)) {
            uproc_class class = key.uint;
            double score = uproc_mosaic_finalize(mosaic);
            if (ctx->filter &&
                !ctx->filter(seq, seq_len, class, score, ctx->filter_arg)) {
                uproc_mosaic_destroy(mosaic);
                continue;
            }
            struct uproc_result result = {
                .rank = i,
                .score = score,
                .class = class,
                .mosaicwords = uproc_mosaic_words_mv(mosaic),
            };

            // TODO: handle errors here
            uproc_list_append(results, &result);
        }
        uproc_bstiter_destroy(iter);
    }
    return res;
}

static int classify(const void *context, const char *seq, uproc_list *results)
{
    const struct context *ctx = context;
    int res = 0;
    uproc_bst *scores[UPROC_RANKS_MAX] = {0};

    int ranks_count = get_ranks_count(ctx);
    for (int i = 0; i < ranks_count; i++) {
        scores[i] = uproc_bst_create(UPROC_BST_UINT, sizeof(uproc_mosaic *));
        if (!scores[i]) {
            goto error;
        }
    }
    res = scores_compute(ctx, seq, scores);
    if (res) {
        goto error;
    }
    res = scores_finalize(ctx, seq, scores, results);

error:
    for (int i = 0; i < ranks_count; i++) {
        uproc_bst_destroy(scores[i]);
    }

    return res;
}

uproc_clf *uproc_clf_create_protein(enum uproc_clf_mode mode, bool detailed,
                                    const uproc_ecurve *fwd,
                                    const uproc_ecurve *rev,
                                    const uproc_substmat *substmat,
                                    uproc_protfilter *filter, void *filter_arg)
{
    struct context *ctx;
    if (!(fwd || rev)) {
        uproc_error_msg(UPROC_EINVAL,
                        "protein classifier requires at least one ecurve");
        return NULL;
    }
    ctx = malloc(sizeof *ctx);
    if (!ctx) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    *ctx = (struct context){
        .detailed = detailed,
        .substmat = substmat,
        .fwd = fwd,
        .rev = rev,
        .filter = filter,
        .filter_arg = filter_arg,
    };
    return uproc_clf_create(mode, ctx, classify, free);
}

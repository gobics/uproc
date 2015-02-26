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
#include "uproc/protclass.h"

#include "ecurve_internal.h"

struct uproc_protclass_s
{
    enum uproc_protclass_mode mode;
    bool detailed;
    const uproc_substmat *substmat;
    const uproc_ecurve *fwd;
    const uproc_ecurve *rev;
    uproc_protfilter *filter;
    void *filter_arg;
    struct uproc_protclass_trace
    {
        uproc_protclass_trace_cb *cb;
        void *cb_arg;
    } trace;
};

/*********************
 * score computation *
 *********************/

struct sc
{
    size_t index;
    double total, dist[UPROC_WORD_LEN];
    uproc_list *matched_words;
};

static inline uproc_rank get_ranks_count(const uproc_protclass *pc)
{
    if (!pc->fwd) {
        return pc->rev->ranks_count;
    }
    return pc->fwd->ranks_count;
}

static void reverse_array(void *p, size_t n, size_t sz)
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

static void sc_init(struct sc *s)
{
    size_t i;
    s->index = -1;
    s->total = 0.0;
    for (i = 0; i < UPROC_WORD_LEN; i++) {
        s->dist[i] = -INFINITY;
    }
    s->matched_words = NULL;
}

static void sc_add(struct sc *score, size_t index,
                   double dist[static UPROC_SUFFIX_LEN], bool reverse)
{
    size_t i, diff;
    double tmp[UPROC_WORD_LEN];

    for (i = 0; i < UPROC_PREFIX_LEN; i++) {
        tmp[i] = -INFINITY;
    }
    memcpy(tmp + UPROC_PREFIX_LEN, dist, sizeof *dist * UPROC_SUFFIX_LEN);
    if (reverse) {
        reverse_array(tmp, UPROC_WORD_LEN, sizeof *tmp);
    }

    if (score->index != (size_t)-1) {
        diff = index - score->index;
        if (diff > UPROC_WORD_LEN) {
            diff = UPROC_WORD_LEN;
        }
        for (i = 0; i < diff; i++) {
            if (isfinite(score->dist[i])) {
                score->total += score->dist[i];
                score->dist[i] = -INFINITY;
            }
        }
    } else {
        diff = 0;
    }

    for (i = 0; i + diff < UPROC_WORD_LEN; i++) {
#define MAX(a, b) (a > b ? a : b)
        score->dist[i] = MAX(score->dist[i + diff], tmp[i]);
    }
    for (; i < UPROC_WORD_LEN; i++) {
        score->dist[i] = tmp[i];
    }
    score->index = index;
}

static void map_list_matchedword_free(void *value, void *opaque)
{
    (void)opaque;
    uproc_matchedword_free(value);
}

static void sc_free(struct sc *s)
{
    if (s->matched_words) {
        uproc_list_map(s->matched_words, map_list_matchedword_free, NULL);
        uproc_list_destroy(s->matched_words);
    }
}

static double sc_finalize(struct sc *score)
{
    size_t i;
    for (i = 0; i < UPROC_WORD_LEN; i++) {
        if (isfinite(score->dist[i])) {
            score->total += score->dist[i];
        }
    }
    return score->total;
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
    struct sc sc;
    sc_init(&sc);
    int res = uproc_bst_get(scores, key, &sc);
    if (res == UPROC_BST_KEY_NOT_FOUND && detailed) {
        sc.matched_words = uproc_list_create(sizeof(struct uproc_matchedword));
        if (!sc.matched_words) {
            return -1;
        }
    }
    sc_add(&sc, index, dist, reverse);
    if (sc.matched_words) {
        struct uproc_matchedword matched_word = {*word, index, reverse, 0.0};
        for (int i = 0; i < UPROC_SUFFIX_LEN; i++) {
            matched_word.score += dist[i];
        }
        int res = uproc_list_append(sc.matched_words, &matched_word);
        if (res) {
            return -1;
        }
    }
    return uproc_bst_update(scores, key, &sc);
}

static int scores_add_word(const uproc_protclass *pc,
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

    int ranks_count = get_ranks_count(pc);
    for (int i = ranks_count; i < UPROC_RANKS_MAX; i++) {
        nb_classes[0][i] = nb_classes[1][i] = UPROC_CLASS_INVALID;
    }

    // Iff both words are equal, we start at index 1 instead of 0
    for (int i = !uproc_word_cmp(&nb_words[0], &nb_words[1]); !res && i < 2;
         i++) {
        uproc_substmat_align_suffixes(substmat, word->suffix,
                                      nb_words[i].suffix, dist);
        if (pc->trace.cb) {
            pc->trace.cb(&nb_words[i], nb_classes[i], index, reverse, dist,
                         pc->trace.cb_arg);
        }
        for (int k = 0; !res && k < ranks_count; k++) {
            uproc_assert(scores[k]);
            res = scores_add(scores[k], &nb_words[i], nb_classes[i][k], index,
                             dist, reverse, pc->detailed);
        }
    }
    return res;
}

static int scores_compute(const struct uproc_protclass_s *pc, const char *seq,
                          uproc_bst *scores[UPROC_RANKS_MAX])
{
    int res;
    uproc_worditer *iter;
    size_t index;
    struct uproc_word fwd_word = UPROC_WORD_INITIALIZER,
                      rev_word = UPROC_WORD_INITIALIZER;

    iter = uproc_worditer_create(seq, uproc_ecurve_alphabet(pc->fwd));
    if (!iter) {
        return -1;
    }

    while (res = uproc_worditer_next(iter, &index, &fwd_word, &rev_word),
           !res) {
        res = scores_add_word(pc, scores, &fwd_word, index, false, pc->fwd,
                              pc->substmat);
        if (res) {
            break;
        }
        res = scores_add_word(pc, scores, &rev_word, index, true, pc->rev,
                              pc->substmat);
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

static int scores_finalize(const struct uproc_protclass_s *pc, const char *seq,
                           uproc_bst *scores[UPROC_RANKS_MAX],
                           uproc_list *results)
{
    int res = 0;
    size_t seq_len = strlen(seq);

    for (int i = 0, n = get_ranks_count(pc); i < n; i++) {
        if (!scores[i] || !uproc_bst_size(scores[i])) {
            continue;
        }
        struct uproc_protresult result = {.rank = i};

        union uproc_bst_key key;
        struct sc sc;
        uproc_bstiter *iter = uproc_bstiter_create(scores[i]);
        if (!iter) {
            return -1;
        }
        while (!uproc_bstiter_next(iter, &key, &sc)) {
            uproc_class class = key.uint;
            double score = sc_finalize(&sc);
            if (pc->filter &&
                !pc->filter(seq, seq_len, class, score, pc->filter_arg)) {
                sc_free(&sc);
                continue;
            }
            result.score = score;
            result.class = class;
            // take ownership -> don't call sc_free.
            result.matched_words = sc.matched_words;
            uproc_list_append(results, &result);
        }
        uproc_bstiter_destroy(iter);
    }
    return res;
}

/**********************
 * exported functions *
 **********************/

uproc_protclass *uproc_protclass_create(enum uproc_protclass_mode mode,
                                        bool detailed, const uproc_ecurve *fwd,
                                        const uproc_ecurve *rev,
                                        const uproc_substmat *substmat,
                                        uproc_protfilter *filter,
                                        void *filter_arg)
{
    struct uproc_protclass_s *pc;
    if (!(fwd || rev)) {
        uproc_error_msg(UPROC_EINVAL,
                        "protein classifier requires at least one ecurve");
        return NULL;
    }
    pc = malloc(sizeof *pc);
    if (!pc) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    *pc = (struct uproc_protclass_s){
        .mode = mode,
        .detailed = detailed,
        .substmat = substmat,
        .fwd = fwd,
        .rev = rev,
        .filter = filter,
        .filter_arg = filter_arg,
        .trace = {
            .cb = NULL, .cb_arg = NULL,
        },
    };
    return pc;
}

void uproc_protclass_destroy(uproc_protclass *pc)
{
    free(pc);
}

static void map_list_protresult_free(void *value, void *opaque)
{
    (void)opaque;
    uproc_protresult_free(value);
}

static void map_list_protresult_max(void *value, void *opaque)
{
    struct uproc_protresult *v = value, *max = opaque;
    if (uproc_protresult_cmp(v, max) > 0) {
        uproc_protresult_free(max);
        uproc_protresult_copy(max, v);
    } else {
        uproc_protresult_free(v);
    }
}

int uproc_protclass_classify(const uproc_protclass *pc, const char *seq,
                             uproc_list **results)
{
    int res;
    uproc_bst *scores[UPROC_RANKS_MAX] = {0};

    int ranks_count = get_ranks_count(pc);
    for (int i = 0; i < ranks_count; i++) {
        scores[i] = uproc_bst_create(UPROC_BST_UINT, sizeof(struct sc));
        if (!scores[i]) {
            goto error;
        }
    }
    if (!*results) {
        *results = uproc_list_create(sizeof(struct uproc_protresult));
        if (!*results) {
            goto error;
        }
    } else {
        uproc_list_map(*results, map_list_protresult_free, NULL);
        uproc_list_clear(*results);
    }

    res = scores_compute(pc, seq, scores);
    if (res) {
        goto error;
    }
    res = scores_finalize(pc, seq, scores, *results);
    if (res) {
        goto error;
    }

    if (pc->mode == UPROC_PROTCLASS_MAX) {
        struct uproc_protresult result_max = UPROC_PROTRESULT_INITIALIZER;
        result_max.rank = UPROC_RANKS_MAX;
        result_max.score = -INFINITY;
        uproc_list_map(*results, map_list_protresult_max, &result_max);
        uproc_list_clear(*results);
        res = uproc_list_append(*results, &result_max);
    }

    if (0) {
    error:
        uproc_list_destroy(*results);
        *results = NULL;
    }
    for (int i = 0; i < ranks_count; i++) {
        uproc_bst_destroy(scores[i]);
    }

    return res;
}

void uproc_protclass_set_trace(uproc_protclass *pc,
                               uproc_protclass_trace_cb *cb, void *cb_arg)
{
    pc->trace.cb = cb;
    pc->trace.cb_arg = cb_arg;
}

void uproc_matchedword_init(struct uproc_matchedword *word)
{
    *word = (struct uproc_matchedword)UPROC_MATCHEDWORD_INITIALIZER;
}

void uproc_matchedword_free(struct uproc_matchedword *word)
{
    (void)word;
}

int uproc_matchedword_copy(struct uproc_matchedword *dest,
                           const struct uproc_matchedword *src)
{
    *dest = *src;
    return 0;
}

void uproc_protresult_init(struct uproc_protresult *results)
{
    *results = (struct uproc_protresult)UPROC_PROTRESULT_INITIALIZER;
}

void uproc_protresult_free(struct uproc_protresult *results)
{
    if (results->matched_words) {
        uproc_list_map(results->matched_words, map_list_matchedword_free, NULL);
        uproc_list_destroy(results->matched_words);
    }
}

int uproc_protresult_copy(struct uproc_protresult *dest,
                          const struct uproc_protresult *src)
{
    *dest = *src;
    // if there was a details list, make a deep copy
    if (src->matched_words) {
        dest->matched_words =
            uproc_list_create(sizeof(struct uproc_matchedword));
        if (!dest->matched_words) {
            return -1;
        }
        for (long i = 0; i < uproc_list_size(src->matched_words); i++) {
            struct uproc_matchedword src_word, dest_word;
            uproc_list_get(src->matched_words, i, &src_word);
            uproc_matchedword_copy(&dest_word, &src_word);
            uproc_list_append(dest->matched_words, &dest_word);
        }
    }
    return 0;
}

int uproc_protresult_cmp(const struct uproc_protresult *p1,
                         const struct uproc_protresult *p2)
{
    int r = p1->rank - p2->rank;
    if (r) {
        return r;
    }
    return ceil(p1->score - p2->score);
}

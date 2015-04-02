#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "uproc/common.h"
#include "uproc/mosaic.h"
#include "uproc/error.h"
#include "uproc/list.h"
#include "util.h"

void uproc_mosaicword_init(struct uproc_mosaicword *mw,
                           const struct uproc_word *word,
                           size_t index,
                           double dist[static UPROC_SUFFIX_LEN],
                           bool reverse)
{
    *mw = (struct uproc_mosaicword){
        .word = *word,
        .index = index,
        .reverse = reverse,
    };
    for (int i = 0; i < UPROC_SUFFIX_LEN; i++) {
        mw->score += dist[i];
    }
}

struct uproc_mosaic_s
{
    size_t index;
    double total, dist[UPROC_WORD_LEN];
    uproc_list *words;  // (struct uproc_mosaicword)
};

uproc_mosaic *uproc_mosaic_create(bool store_words)
{
    uproc_mosaic *m = malloc(sizeof *m);
    if (!m) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    *m = (struct uproc_mosaic_s){
        .index = -1,
    };
    for (int i = 0; i < UPROC_WORD_LEN; i++) {
        m->dist[i] = -INFINITY;
    }
    if (store_words) {
        m->words = uproc_list_create(sizeof (struct uproc_mosaicword));
        if (!m->words) {
            goto error;
        }
    }
    return m;
error:
    free(m);
    return NULL;
}

void uproc_mosaic_destroy(uproc_mosaic *m)
{
    uproc_list_destroy(m->words);
    free(m);
}

int uproc_mosaic_add(uproc_mosaic *m, const struct uproc_word *w, size_t index,
                     double dist[static UPROC_SUFFIX_LEN], bool reverse)
{
    if (m->words && w) {
        struct uproc_mosaicword mw;
        uproc_mosaicword_init(&mw, w, index, dist, reverse);
        int res = uproc_list_append(m->words, &mw);
        if (res) {
            return res;
        }
    }

    // Number of elements in the current `dist` array that are known to be part
    // of the final mosaic score. This is the difference between `index` of the
    // last added and newly added word, but is at most UPROC_SUFFIX_LEN.
    size_t diff = 0;
    if (m->index != (size_t)-1) {
        uproc_assert(index >= m->index);
        diff = index - m->index;
        if (diff > UPROC_WORD_LEN) {
            diff = UPROC_WORD_LEN;
        }
        for (size_t i = 0; i < diff; i++) {
            if (isfinite(m->dist[i])) {
                m->total += m->dist[i];
                m->dist[i] = -INFINITY;
            }
        }
    }

    // Pad `dist` with -inf so it has the same number of elements as `m->dist`.
    // This makes the loop below easy to read, and to add a reverse word we
    // just need to reverse the `tmp` array.
    double ninf = -INFINITY, tmp[UPROC_WORD_LEN];
    array_set(tmp, UPROC_PREFIX_LEN, sizeof *tmp, &ninf);
    memcpy(tmp + UPROC_PREFIX_LEN, dist, sizeof *dist * UPROC_SUFFIX_LEN);
    if (reverse) {
        array_reverse(tmp, UPROC_WORD_LEN, sizeof *tmp);
    }

    array_shiftleft(m->dist, UPROC_WORD_LEN, sizeof *m->dist, diff, &ninf);
    for (int i = 0; i < UPROC_WORD_LEN; i++) {
        m->dist[i] = fmax(m->dist[i], tmp[i]);
    }
    m->index = index;
    return 0;
}

double uproc_mosaic_finalize(uproc_mosaic *m)
{
    for (int i = 0; i < UPROC_WORD_LEN; i++) {
        if (isfinite(m->dist[i])) {
            m->total += m->dist[i];
        }
        m->dist[i] = -INFINITY;
    }
    m->index = -1;
    double t = m->total;
    m->total = 0.0;
    return t;
}

uproc_list *uproc_mosaic_words_mv(uproc_mosaic *m)
{
    uproc_list *w = m->words;
    m->words = NULL;
    return w;
}

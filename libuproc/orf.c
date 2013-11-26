/* Translate DNA/RNA to protein sequence
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <ctype.h>

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/codon.h"
#include "uproc/matrix.h"
#include "uproc/orf.h"
#include "uproc/io.h"

#include "codon_tables.h"

#define FRAMES (UPROC_ORF_FRAMES / 2)
#define BUFSZ_INIT 2
#define BUFSZ_STEP 50


static void
reverse_str(char *s)
{
    char tmp, *p;
    size_t len = strlen(s);

    if (!len) {
        return;
    }

    for (p = s + len - 1; s < p; s++, p--) {
        tmp = *s;
        *s = *p;
        *p = tmp;
    }
}

static uproc_codon
scoreindex_to_codon(int idx)
{
    unsigned i = 3;
    uproc_codon c = 0;
    while (i--) {
        uproc_codon_prepend(&c, 1 << (idx & 0x3));
        idx >>= 2;
    }
    return c;
}

static int
orf_add_codon(struct uproc_orf *o, size_t *sz, uproc_codon c, double score)
{
    if (!o->length && CODON_TO_CHAR(c) == 'X') {
        return 0;
    }
    if (o->length + 1 == *sz) {
        char *tmp = realloc(o->data, *sz + BUFSZ_STEP);
        if (!tmp) {
            return uproc_error(UPROC_ENOMEM);
        }
        o->data = tmp;
        *sz += BUFSZ_STEP;
    }
    o->data[o->length] = CODON_TO_CHAR(c);
    o->length++;
    o->score += score;
    return 0;
}

static void
gc_content(const char *seq, size_t *len, double *gc)
{
    static double gc_map[UCHAR_MAX + 1] = {
        ['G'] = 1,    ['C'] = 1,
        ['R'] = .5,   ['Y'] = .5,   ['S'] = 1,    ['K'] = .5,   ['M'] = .5,
        ['B'] = .667, ['D'] = .333, ['H'] = .333, ['V'] = .667,
        ['N'] = .25,
    };
    size_t i;
    double count = 0.0;
    for (i = 0; seq[i]; i++) {
        count += gc_map[toupper(seq[i])];
    }
    *gc = count / i;
    *len = i;
}

void
uproc_orf_codonscores(double scores[static UPROC_BINARY_CODON_COUNT],
        const struct uproc_matrix *score_matrix)
{
    uproc_codon c1, c2;
    for (c1 = 0; c1 < UPROC_BINARY_CODON_COUNT; c1++) {
        unsigned i, count = 0;
        double sum = 0.0;
        if (score_matrix) {
            for (i = 0; i < UPROC_CODON_COUNT; i++) {
                c2 = scoreindex_to_codon(i);
                if (!CODON_IS_STOP(c2) && uproc_codon_match(c2, c1)) {
                    sum += uproc_matrix_get(score_matrix, i, 0);
                    count++;
                }
            }
            scores[c1] = count ? sum / count : 0.0;
        }
        else {
            scores[c1] = 0.0;
        }
    }
}

int
uproc_orfiter_init(struct uproc_orfiter *iter, const char *seq,
        const double *codon_scores, uproc_orf_filter *filter, void *filter_arg)
{
    unsigned i;
    *iter = (struct uproc_orfiter) {
            .seq = seq,
            .pos = seq,
            .filter = filter,
            .filter_arg = filter_arg,
    };
    gc_content(seq, &iter->seq_len, &iter->seq_gc);

    for (i = 0; i < UPROC_ORF_FRAMES; i++) {
        iter->data_sz[i] = BUFSZ_INIT;
        iter->orf[i].data = malloc(BUFSZ_INIT);
        if (!iter->orf[i].data) {
            while (i--) {
                free(iter->orf[i].data);
            }
            return uproc_error(UPROC_ENOMEM);
        }
        iter->orf[i].frame = i;
    }
    iter->codon_scores = codon_scores;
    return 0;
}

void
uproc_orfiter_destroy(struct uproc_orfiter *iter)
{
    unsigned i;
    for (i = 0; i < UPROC_ORF_FRAMES; i++) {
        free(iter->orf[i].data);
    }
}

int
uproc_orfiter_next(struct uproc_orfiter *iter, struct uproc_orf *next)
{
    unsigned i;
    uproc_nt nt;
    uproc_codon c_fwd, c_rev;

    while (true) {
        /* yield finished ORFs */
        for (i = 0; i < UPROC_ORF_FRAMES; i++) {
            if (!iter->yield[i]) {
                continue;
            }

            *next = iter->orf[i];

            /* reinitialize working ORF */
            iter->orf[i].length = 0;
            iter->orf[i].score = 0;
            iter->yield[i] = false;

            /* chop trailing wildcard aminoacids */
            while (next->length && next->data[next->length - 1] == 'X') {
                next->length--;
            }
            if (!next->length) {
                continue;
            }
            if (iter->filter && !iter->filter(next, iter->seq, iter->seq_len,
                        iter->seq_gc, iter->filter_arg)) {
                continue;
            }

            next->data[next->length] = '\0';

            /* revert ORF on complementery strand */
            if (i >= FRAMES) {
                reverse_str(next->data);
            }
            return 1;
        }

        /* iterator exhausted */
        if (iter->frame >= FRAMES) {
            return 0;
        }

        /* sequence completely processed, yield all ORFs */
        if (!iter->pos) {

            if (iter->nt_count > iter->frame) {
                iter->yield[iter->frame] =
                    iter->yield[iter->frame + FRAMES] = true;
            }
            iter->frame++;
            continue;
        }

        /* process sequence */
        if (*iter->pos) {
            nt = CHAR_TO_NT(*iter->pos++);
            if (nt == UPROC_NT_NOT_CHAR) {
                continue;
            }
            if (nt == UPROC_NT_NOT_IUPAC) {
                nt = CHAR_TO_NT('N');
            }
            iter->nt_count++;
            iter->frame = (iter->frame + 1) % FRAMES;

            for (i = 0; i < FRAMES; i++) {
                uproc_codon_append(&iter->codon[i], nt);
            }

            /* skip partially read codons */
            if (iter->nt_count < FRAMES) {
                continue;
            }

#define ADD_CODON(codon, frame) do {                                        \
    int res = orf_add_codon(&iter->orf[(frame)], &iter->data_sz[(frame)],   \
            codon,                                                          \
            iter->codon_scores ?  iter->codon_scores[codon] : 0.0);         \
    if (res) {                                                              \
        return res;                                                         \
    }                                                                       \
} while (0)

            c_fwd = iter->codon[iter->frame];
            if (CODON_IS_STOP(c_fwd)) {
                iter->yield[iter->frame] = true;
            }
            else {
                ADD_CODON(c_fwd, iter->frame);
            }

            c_rev = CODON_COMPLEMENT(c_fwd);
            if (CODON_IS_STOP(c_rev)) {
                iter->yield[iter->frame + FRAMES] = true;
            }
            else {
                ADD_CODON(c_rev, iter->frame + FRAMES);
            }
        }
        else {
            /* guess last nt for the next frame */
            unsigned frame = (iter->nt_count + 1) % FRAMES;
            c_fwd = iter->codon[frame];
            uproc_codon_append(&c_fwd, UPROC_NT_N);
            c_rev = CODON_COMPLEMENT(c_fwd);
            if (!CODON_IS_STOP(c_fwd) && CODON_TO_CHAR(c_fwd) != 'X') {
                ADD_CODON(c_fwd, frame);
            }
            if (!CODON_IS_STOP(c_rev) && CODON_TO_CHAR(c_rev) != 'X') {
                ADD_CODON(c_rev, frame + FRAMES);
            }

            /* "signal" that the end of the sequence was reached */
            iter->frame = 0;
            iter->pos = NULL;
        }
    }
    return -1;
}

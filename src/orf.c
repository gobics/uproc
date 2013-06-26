#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include "ecurve/common.h"
#include "ecurve/codon.h"
#include "ecurve/matrix.h"
#include "ecurve/orf.h"
#include "codon_tables.h"

#define FRAMES (EC_ORF_FRAMES / 2)
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

static ec_codon
scoreindex_to_codon(int idx)
{
    unsigned i = 3;
    ec_codon c = 0;
    while (i--) {
        ec_codon_prepend(&c, 1 << (idx & 0x3));
        idx >>= 2;
    }
    return c;
}

static int
orf_add_codon(struct ec_orf *o, size_t *sz, ec_codon c, double score)
{
    if (!o->length && CODON_TO_CHAR(c) == 'X') {
        return EC_SUCCESS;
    }
    if (o->length + 1 == *sz) {
        char *tmp = realloc(o->data, *sz + BUFSZ_STEP);
        if (!tmp) {
            return EC_FAILURE;
        }
        o->data = tmp;
        *sz += BUFSZ_STEP;
    }
    o->data[o->length] = CODON_TO_CHAR(c);
    o->length++;
    o->score += score;
    return EC_SUCCESS;
}

static void
gc_content(const char *seq, double *gc, size_t *len)
{
    static unsigned gc_map[UCHAR_MAX + 1] = {
        [(unsigned char)'G'] = 1,
        [(unsigned char)'C'] = 1,
        [(unsigned char)'g'] = 1,
        [(unsigned char)'c'] = 1,
    };
    size_t i;
    unsigned count;

    for (i = count = 0; seq[i]; i++) {
        count += gc_map[(unsigned char)seq[i]];
    }
    *gc = (double) count / i;
    *len = i;
}

static double
get_threshold(const ec_matrix *thresholds, const char *seq)
{
    double gc;
    size_t r, c, rows, cols;

    gc_content(seq, &gc, &c);
    ec_matrix_dimensions(thresholds, &rows, &cols);

    r = gc * 100;
    if (r >= rows) {
        r = rows - 1;
    }
    if (c >= cols) {
        c = cols - 1;
    }
    return ec_matrix_get(thresholds, r, c);
}

int
ec_orf_codonscores_load_file(ec_orf_codonscores *scores, const char *path)
{
    int res;
    ec_matrix m;

    res = ec_matrix_load_file(&m, path);
    if (res != EC_SUCCESS) {
        return res;
    }
    ec_orf_codonscores_init(scores, &m);
    ec_matrix_destroy(&m);
    return EC_SUCCESS;
}

int
ec_orf_codonscores_load_stream(ec_orf_codonscores *scores, FILE *stream)
{
    int res;
    ec_matrix m;

    res = ec_matrix_load_stream(&m, stream);
    if (res != EC_SUCCESS) {
        return res;
    }
    ec_orf_codonscores_init(scores, &m);
    ec_matrix_destroy(&m);
    return EC_SUCCESS;
}

void
ec_orf_codonscores_init(ec_orf_codonscores *scores,
                        const ec_matrix *score_matrix)
{
    ec_codon c1, c2;
    struct ec_orf_codonscores_s *s = scores;
    for (c1 = 0; c1 < EC_BINARY_CODON_COUNT; c1++) {
        unsigned i, count = 0;
        double sum = 0.0;
        if (score_matrix) {
            for (i = 0; i < EC_CODON_COUNT; i++) {
                c2 = scoreindex_to_codon(i);
                if (!CODON_IS_STOP(c2) && ec_codon_match(c2, c1)) {
                    sum += ec_matrix_get(score_matrix, i, 0);
                    count++;
                }
            }
            s->values[c1] = count ? sum / count : 0.0;
        }
        else {
            s->values[c1] = 0.0;
        }
    }
}

int
ec_orfiter_init(ec_orfiter *iter, const char *seq,
                const ec_orf_codonscores *codon_scores)
{
    unsigned i;
    struct ec_orfiter_s *it = iter;
    *it = (struct ec_orfiter_s) { .pos = seq };

    for (i = 0; i < EC_ORF_FRAMES; i++) {
        it->data_sz[i] = BUFSZ_INIT;
        it->orf[i].data = malloc(BUFSZ_INIT);
        if (!it->orf[i].data) {
            while (i--) {
                free(it->orf[i].data);
            }
            return EC_FAILURE;
        }
    }
    it->codon_scores = codon_scores;
    return EC_SUCCESS;
}

void
ec_orfiter_destroy(ec_orfiter *iter)
{
    unsigned i;
    struct ec_orfiter_s *it = iter;
    for (i = 0; i < EC_ORF_FRAMES; i++) {
        free(it->orf[i].data);
    }
}

int
ec_orfiter_next(ec_orfiter *iter, struct ec_orf *next, unsigned *frame)
{
    int res;
    unsigned i;
    ec_nt nt;
    ec_codon c_fwd, c_rev;
    struct ec_orfiter_s *it = iter;

    while (true) {
        /* yield finished ORFs */
        for (i = 0; i < EC_ORF_FRAMES; i++) {
            if (!it->yield[i]) {
                continue;
            }

            *next = it->orf[i];
            *frame = i;

            /* reinitialize working ORF */
            it->orf[i].length = 0;
            it->orf[i].score = 0;
            it->yield[i] = false;

            /* chop trailing wildcard aminoacids */
            while (next->length && next->data[next->length - 1] == 'X') {
                next->length--;
            }
            if (!next->length) {
                continue;
            }
            next->data[next->length] = '\0';

            /* revert ORF on complementery strand */
            if (i >= FRAMES) {
                reverse_str(next->data);
            }

            return EC_SUCCESS;
        }

        /* iterator exhausted */
        if (it->frame >= FRAMES) {
            assert(!it->pos);
            return EC_ITER_STOP;
        }

        /* sequence completely processed, yield all ORFs */
        if (!it->pos) {

            if (it->nt_count > it->frame) {
                it->yield[it->frame] = it->yield[it->frame + FRAMES] = true;
            }
            it->frame++;
            continue;
        }

        /* process sequence */
        if (*it->pos) {
            nt = CHAR_TO_NT(*it->pos++);
            if (nt == EC_NT_NOT_CHAR) {
                continue;
            }
            if (nt == EC_NT_NOT_IUPAC) {
                nt = CHAR_TO_NT('N');
            }
            it->nt_count++;
            it->frame = (it->frame + 1) % FRAMES;
            assert(it->nt_count % FRAMES == it->frame);

            for (i = 0; i < FRAMES; i++) {
                ec_codon_append(&it->codon[i], nt);
            }

            /* skip partially read codons */
            if (it->nt_count < FRAMES) {
                continue;
            }

#define ADD_CODON(codon, frame) do {                                        \
    res = orf_add_codon(&it->orf[(frame)], &it->data_sz[(frame)], codon,    \
                        it->codon_scores ?                                  \
                            it->codon_scores->values[codon] : 0.0);         \
    if (res != EC_SUCCESS) {                                                \
        return EC_FAILURE;                                                  \
    }                                                                       \
} while (0)

            c_fwd = it->codon[it->frame];
            if (CODON_IS_STOP(c_fwd)) {
                it->yield[it->frame] = true;
            }
            else {
                ADD_CODON(c_fwd, it->frame);
            }

            c_rev = CODON_COMPLEMENT(c_fwd);
            if (CODON_IS_STOP(c_rev)) {
                it->yield[it->frame + FRAMES] = true;
            }
            else {
                ADD_CODON(c_rev, it->frame + FRAMES);
            }
        }
        else {
            /* guess last nt for the next frame */
            unsigned frame = (it->nt_count + 1) % FRAMES;
            c_fwd = it->codon[frame];
            ec_codon_append(&c_fwd, EC_NT_N);
            c_rev = CODON_COMPLEMENT(c_fwd);
            if (!CODON_IS_STOP(c_fwd) && CODON_TO_CHAR(c_fwd) != 'X') {
                ADD_CODON(c_fwd, frame);
            }
            if (!CODON_IS_STOP(c_rev) && CODON_TO_CHAR(c_rev) != 'X') {
                ADD_CODON(c_rev, frame + FRAMES);
            }

            /* "signal" that the end of the sequence was reached */
            it->frame = 0;
            it->pos = NULL;
        }
    }
    assert(0);
    return EC_FAILURE;
}

int
ec_orf_chained(const char *seq,
               enum ec_orf_mode mode,
               const ec_orf_codonscores *codon_scores,
               const ec_matrix *thresholds,
               char **buf, size_t *sz)
{
    int res;
    size_t len_total[EC_ORF_FRAMES] = { 0 };
    struct ec_orf orf;
    unsigned frame;
    ec_orfiter iter;
    double min_score = 0.0;

    if (thresholds) {
        min_score = get_threshold(thresholds, seq);
    }
    else {
        min_score = -EC_INFINITY;
    }

    res = ec_orfiter_init(&iter, seq, codon_scores);
    if (res != EC_SUCCESS) {
        return EC_FAILURE;
    }

    while ((res = ec_orfiter_next(&iter, &orf, &frame)) == EC_SUCCESS) {
        char *p;
        size_t len_new;
        if (orf.length < EC_WORD_LEN || orf.score < min_score) {
            continue;
        }
        frame %= mode;
        len_new = len_total[frame] + orf.length + 1;

        /* resize buffer if necessary */
        if (!buf[frame] || len_new > sz[frame]) {
            p = realloc(buf[frame], len_new);
            if (!p) {
                res = EC_FAILURE;
                goto error;
            }
            buf[frame] = p;
            sz[frame] = len_new;
        }

        p = buf[frame] + len_total[frame];
        if (len_total[frame]) {
            p[-1] = EC_ORF_SEPARATOR;
        }
        strcpy(p, orf.data);
        len_total[frame] = len_new;
    }
    res = EC_SUCCESS;

error:
    ec_orfiter_destroy(&iter);
    return res;
}

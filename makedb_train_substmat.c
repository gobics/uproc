/* uproc-makedb
 * Create a new uproc database.
 *
 * Copyright 2014 Peter Meinicke, Robin Martinjak
 *
 * This file is part of uproc.
 *
 * uproc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * uproc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with uproc.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <uproc.h>
#include "libuproc/ecurve_internal.h"

#define N_DIMS (UPROC_ALPHABET_SIZE * UPROC_ALPHABET_SIZE * UPROC_SUFFIX_LEN)
#define ELEMENTS(x) (sizeof(x) / sizeof(x)[0])

#if 1
#define LOGF(fmt, ...) fprintf(stderr, fmt "\n", __VA_ARGS__)
#define LOG(msg) fprintf(stderr, "%s\n", msg)
#else
#define LOGF(fmt, ...)
#define LOG(msg)
#endif

uproc_class first_class(const uproc_ecurve *ec, size_t idx)
{
    uproc_class cls[UPROC_RANKS_MAX];
    uproc_ecurve_get_classes(ec, idx, cls);
    return cls[0];
}

enum ft {
    INVALID_CLASS,
    BOTH_EQUAL,
    BOTH_DIFFER,
    LO_DIFFERS,
    HI_DIFFERS,
};

int feature_type(const uproc_ecurve *ec, size_t idx)
{
    uproc_class lo = first_class(ec, idx - 1),
                mid = first_class(ec, idx),
                hi = first_class(ec, idx + 1);

    if (mid == UPROC_CLASS_INVALID) {
        return INVALID_CLASS;
    }
    if ((mid == lo) == (mid == hi)) {
        return (mid == lo) ? BOTH_EQUAL : BOTH_DIFFER;
    }
    if (mid == lo) {
        return HI_DIFFERS;
    }
    return LO_DIFFERS;
}

#define AMINO_AT(x, n) \
    (((x) >> (UPROC_AMINO_BITS * (UPROC_SUFFIX_LEN - (n) - 1))) \
     & UPROC_BITMASK(UPROC_AMINO_BITS))

static unsigned make_index(uproc_suffix a, uproc_suffix b, unsigned i)
{
    uproc_amino aa = AMINO_AT(a, i);
    uproc_amino ab = AMINO_AT(b, i);
    unsigned idx =
        i*UPROC_ALPHABET_SIZE*UPROC_ALPHABET_SIZE +
        aa*UPROC_ALPHABET_SIZE + ab;
    return idx;
}

static unsigned long n_features;
static void add_feature(uproc_matrix *vec, uproc_matrix *mat,
                        uproc_suffix a, uproc_suffix b, double increment)
{
    unsigned indices[UPROC_SUFFIX_LEN];
    n_features++;

    for (unsigned i = 0; i < UPROC_SUFFIX_LEN; i++) {
        unsigned idx = make_index(a, b, i);
        uproc_matrix_add_elem(vec, idx, 0, increment);
        indices[i] = idx;
    }
    for (unsigned i = 0; i < UPROC_SUFFIX_LEN; i++) {
        for (unsigned j = 0; j < UPROC_SUFFIX_LEN; j++) {
            uproc_matrix_add_elem(mat, indices[i], indices[j], 1.0);
        }
    }
}

static void count_features(const uproc_ecurve *ec, uproc_matrix *vec,
                           uproc_matrix *mat)
{
    uproc_assert(ec->suffix_count >= 3);

    for (size_t i = 1; i < ec->suffix_count - 1; i++) {
        int ft = feature_type(ec, i);
        if (ft != LO_DIFFERS && ft != HI_DIFFERS) {
            continue;
        }
        uproc_suffix lo = ec->suffixes[i-1],
                     mid = ec->suffixes[i],
                     hi = ec->suffixes[i+1];
        add_feature(vec, mat, mid, lo, (ft == LO_DIFFERS ? -1 : 1));
        add_feature(vec, mat, mid, hi, (ft == HI_DIFFERS ? -1 : 1));
    }
    LOGF("%lu features", n_features);
}

static uproc_substmat *vec_to_substmat(const uproc_matrix *vec)
{
    unsigned long rows, cols;
    uproc_matrix_dimensions(vec, &rows, &cols);
    uproc_assert(rows == N_DIMS);
    uproc_assert(cols == 1);

    uproc_substmat *substmat = uproc_substmat_create();

    for (unsigned i = 0; i < UPROC_SUFFIX_LEN; i++) {
        for (uproc_amino a = 0; a < UPROC_ALPHABET_SIZE; a++) {
            for (uproc_amino b = 0; b < UPROC_ALPHABET_SIZE; b++) {
                unsigned idx =
                    i*UPROC_ALPHABET_SIZE*UPROC_ALPHABET_SIZE +
                    a*UPROC_ALPHABET_SIZE + b;
                double v = uproc_matrix_get(vec, idx, 0);
                uproc_substmat_set(substmat, i, a, b, v);
            }
        }
    }
    return substmat;
}

static uproc_substmat *make_candidate(const uproc_matrix *features,
                                      const uproc_matrix *coocc,
                                      double lambda)
{
    LOGF("make_candidate(%g)", lambda);
    uproc_matrix *lambdas = uproc_matrix_eye(N_DIMS, lambda);

    uproc_matrix *A = uproc_matrix_add(coocc, lambdas);
    uproc_matrix_destroy(lambdas);

    uproc_matrix *x = uproc_matrix_ldivide(A, features);
    uproc_matrix_destroy(A);

    uproc_matrix_store(x, UPROC_IO_STDIO, "substmat_%g_as_matrix", lambda);
    uproc_substmat *substmat = vec_to_substmat(x);
    uproc_matrix_destroy(x);
    return substmat;
}

struct stats {
    unsigned long tp, fp, fn;
};

static double dist_sum(double dist[static UPROC_SUFFIX_LEN])
{
    double s = 0.0;
    for (int i = 0; i < UPROC_SUFFIX_LEN; i++) {
        s += dist[i];
    }
    return s;
}

static void score_candidate_ec(const uproc_ecurve *ec,
                               const uproc_substmat *substmat,
                               struct stats *stats)
{
    uproc_assert(ec->suffix_count >= 3);

    for (size_t i = 1; i < ec->suffix_count - 1; i++) {
        int ft = feature_type(ec, i);
        if (ft != BOTH_EQUAL && ft != BOTH_DIFFER) {
            continue;
        }

        double dist[UPROC_SUFFIX_LEN];
        uproc_suffix lo = ec->suffixes[i-1],
                     mid = ec->suffixes[i],
                     hi = ec->suffixes[i+1];
        uproc_substmat_align_suffixes(substmat, mid, lo, dist);
        double score_lo = dist_sum(dist);
        uproc_substmat_align_suffixes(substmat, mid, hi, dist);
        double score_hi = dist_sum(dist);

        if (score_lo > 0 || score_hi > 0) {
            if (ft == BOTH_EQUAL)  {
                stats->tp++;
            } else {
                stats->fp++;
            }
        } else {
            if (ft == BOTH_EQUAL) {
                stats->fn++;
            }
            // true negatives are irrelevant
        }
    }
}

static double score_candidate(const uproc_ecurve *fwd, const uproc_ecurve *rev,
                              const uproc_substmat *substmat)
{
    struct stats stats = { 0, 0, 0 };
    score_candidate_ec(fwd, substmat, &stats);
    score_candidate_ec(rev, substmat, &stats);
    LOGF("tp: %lu\tfp: %lu\tfn: %lu", stats.tp, stats.fp, stats.fn);
    return (2.0 * stats.tp) / (2.0 * stats.tp + stats.fp + stats.fn);
}

uproc_substmat *train_substmat(const uproc_ecurve *fwd, const uproc_ecurve *rev)
{
    LOG("hello");

    uproc_error_set_handler(NULL, NULL);
    uproc_matrix *coocc = uproc_matrix_load(UPROC_IO_GZIP, "coocc");
    uproc_matrix *features = uproc_matrix_load(UPROC_IO_GZIP, "features");
    uproc_error_set_handler(errhandler_bail, NULL);

    if (!coocc) {
        coocc = uproc_matrix_create(N_DIMS, N_DIMS, NULL);
        features = uproc_matrix_create(N_DIMS, 1, NULL);

        LOG("counting features fwd");
        count_features(fwd, features, coocc);
        LOG("counting features rev");
        count_features(rev, features, coocc);
        uproc_matrix_store(coocc, UPROC_IO_STDIO, "coocc");
        uproc_matrix_store(features, UPROC_IO_STDIO, "features");
    }

    double lambdas[] = { 1e8, 1e7, 1e6, 1e5, 1e4, };

    // the substitution matrix that obtains the best f1 score wins
    double f1_max = 0.0;
    uproc_substmat *best_candidate = NULL;

    for (unsigned i = 0; i < ELEMENTS(lambdas); i++) {
        uproc_substmat *cand = make_candidate(features, coocc,
                                              lambdas[i]);
        uproc_substmat_store(cand, UPROC_IO_STDIO, "substmat_%g", lambdas[i]);
        double f1 = score_candidate(fwd, rev, cand);
        LOGF("f1: %g", f1);
        if (f1 > f1_max || !best_candidate) {
            f1 = f1_max;
            uproc_substmat_destroy(best_candidate);
            best_candidate = cand;
        } else {
            uproc_substmat_destroy(cand);
        }
    }
    LOG("bye");
    return best_candidate;
}

#ifdef TEST
uproc_alphabet *alpha_;

struct entry {
    char w[UPROC_WORD_LEN + 2];
    uproc_class cls;
};

void add_entries(struct uproc_ecurve_s *ec, struct entry *entries, size_t n)
{
    ec->ranks_count = 1;

    ec->suffixes = malloc(sizeof *ec->suffixes * n);
    ec->classes = malloc(sizeof *ec->classes * n);
    ec->suffix_count = n;

    for (size_t i = 0; i < n; i++) {
        struct uproc_word w = { 0, 0 };
        uproc_word_from_string(&w, entries[i].w, alpha_);
        ec->suffixes[i] = w.suffix;
        ec->classes[i] = entries[i].cls;
    }
}


void test_count_features(void)
{
    struct uproc_ecurve_s fwd = { 0 }, rev = { 0 };

    struct entry entries_fwd[] = {
        { "PPPPPPAAAAAAAAAAAA", 0 },
        { "PPPPPPAAAAAAAAAAAG", 0 },
        { "PPPPPPGAAAAAAAAAGG", 1 },
        { "PPPPPPGAAAAAAAAAGG", 1 },
        { "PPPPPPGAAAAAAAAGGG", 1 },
    };
    struct entry entries_rev[] = {
        { "PPPPPPSSSSSSSSSSSS", 0 },
        { "PPPPPPSSSSSSSSSSSG", 0 },
        { "PPPPPPGSSSSSSSSSGG", 1 },
        { "PPPPPPGSSSSSSSSSGG", 1 },
        { "PPPPPPGTTTTTTTTGGG", 1 },
    };

#define ADD_ENTRIES(ec, entries) add_entries((ec), (entries), ELEMENTS((entries)))
    ADD_ENTRIES(&fwd, entries_fwd);
    ADD_ENTRIES(&rev, entries_rev);
#undef ADD_ENTRIES

    uproc_matrix *coocc = uproc_matrix_create(N_DIMS, N_DIMS, NULL);
    uproc_matrix *features = uproc_matrix_create(N_DIMS, 1, NULL);
    count_features(&fwd, features, coocc);
    uproc_assert(
        uproc_matrix_get(features, 0, 0) == +1);
    uproc_assert(
        uproc_matrix_get(features, 1, 0) == -1);
    uproc_assert(
        uproc_matrix_get(features, 20, 0) == -1);
    uproc_assert(
        uproc_matrix_get(features, 21, 0) == +1);

    uproc_matrix_destroy(features);
    uproc_matrix_destroy(coocc);
}


int main(void) {
    uproc_error_set_handler(errhandler_bail, NULL);
    alpha_ = uproc_alphabet_create("AGSTPKRQEDNHYWFMLIVC");

    test_count_features();
    return 0;
}
#endif

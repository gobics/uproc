/** \file uproc/dnaclass.h
 *
 * Classify DNA/RNA sequences
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

#ifndef UPROC_DNACLASS_H
#define UPROC_DNACLASS_H

#include "uproc/orf.h"
#include "uproc/protclass.h"
#include "uproc/matrix.h"

enum uproc_dc_mode
{
    UPROC_DC_ALL,

    UPROC_DC_MAX,
};

struct uproc_dc_results
{
    struct uproc_dc_pred
    {
        uproc_family family;
        double score;
        unsigned frame;
    } *preds;
    size_t n, sz;
};

#define UPROC_DC_RESULTS_INITIALIZER { NULL, 0, 0 }

struct uproc_dnaclass
{
    enum uproc_dc_mode mode;

    const struct uproc_protclass *pc;

    double codon_scores[UPROC_BINARY_CODON_COUNT];
    uproc_orf_filter *orf_filter;
    void *orf_filter_arg;
};

int uproc_dc_init(struct uproc_dnaclass *dc,
                  enum uproc_dc_mode mode,
                  const struct uproc_protclass *pc,
                  const struct uproc_matrix *codon_scores,
                  uproc_orf_filter *orf_filter, void *orf_filter_arg);

int uproc_dc_classify(const struct uproc_dnaclass *dc, const char *seq,
                      struct uproc_dc_results *results);
#endif

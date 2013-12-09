/** \file uproc/protclass.h
 *
 * Classify protein sequences
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

#ifndef UPROC_PROTCLASS_H
#define UPROC_PROTCLASS_H


#include "uproc/common.h"
#include "uproc/ecurve.h"
#include "uproc/substmat.h"

typedef bool uproc_pc_filter(const char*, size_t, uproc_family, double, void*);

enum uproc_pc_mode
{
    UPROC_PC_ALL,
    UPROC_PC_MAX,
};

struct uproc_pc_results
{
    struct uproc_pc_pred
    {
        uproc_family family;
        double score;
    } *preds;
    size_t n, sz;
};

#define UPROC_PC_RESULTS_INITIALIZER { NULL, 0, 0 }

struct uproc_protclass
{
    enum uproc_pc_mode mode;
    const struct uproc_substmat *substmat;
    const struct uproc_ecurve *fwd;
    const struct uproc_ecurve *rev;
    uproc_pc_filter *filter;
    void *filter_arg;
};

int uproc_pc_init(struct uproc_protclass *pc, enum uproc_pc_mode mode,
                  const struct uproc_ecurve *fwd,
                  const struct uproc_ecurve *rev,
                  const struct uproc_substmat *substmat,
                  uproc_pc_filter *filter, void *filter_arg);

void uproc_pc_destroy(struct uproc_protclass *pc);

int uproc_pc_classify(const struct uproc_protclass *pc, const char *seq,
                      struct uproc_pc_results *results);
#endif
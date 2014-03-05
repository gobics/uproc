/* Copyright 2014 Peter Meinicke, Robin Martinjak
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

/** \file uproc/protclass.h
 *
 *
 * \weakgroup grp_clf
 * \{
 * \weakgroup grp_clf_prot
 *
 * \{
 */
#ifndef UPROC_PROTCLASS_H
#define UPROC_PROTCLASS_H


#include "uproc/common.h"
#include "uproc/ecurve.h"
#include "uproc/substmat.h"

typedef bool uproc_protfilter(const char*, size_t, uproc_family, double, void*);

enum uproc_protclass_mode
{
    UPROC_PROTCLASS_ALL,
    UPROC_PROTCLASS_MAX,
};

struct uproc_protresults
{
    struct uproc_protpred
    {
        uproc_family family;
        double score;
    } *preds;
    size_t n, sz;
};

#define UPROC_PROTRESULTS_INITIALIZER { NULL, 0, 0 }

typedef struct uproc_protclass_s uproc_protclass;

uproc_protclass *uproc_protclass_create(enum uproc_protclass_mode mode,
                                        const uproc_ecurve *fwd,
                                        const uproc_ecurve *rev,
                                        const uproc_substmat *substmat,
                                        uproc_protfilter *filter,
                                        void *filter_arg);

void uproc_protclass_destroy(uproc_protclass *pc);

int uproc_protclass_classify(const uproc_protclass *pc, const char *seq,
                             struct uproc_protresults *results);

typedef void uproc_protclass_trace_cb(const char *pfx, const char *sfx,
                                      size_t index, bool reverse,
                                      const double *scores, void *opaque);

void uproc_protclass_set_trace(uproc_protclass *pc, uproc_family family,
                               uproc_protclass_trace_cb *cb, void *cb_arg);

void uproc_protresults_init(struct uproc_protresults *results);

void uproc_protresults_free(struct uproc_protresults *results);

int uproc_protresults_copy(struct uproc_protresults *dest,
                          const struct uproc_protresults *src);
/**
 * \}
 * \}
 */
#endif

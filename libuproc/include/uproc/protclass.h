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


/** Protein classification results
 *
* \defgroup struct_protresults struct uproc_protresults
* \{
 */
struct uproc_protresults
{
    struct uproc_protpred
    {
        uproc_family family;
        double score;
    } *preds;
    size_t n, sz;
};

/** Initializer for ::uproc_protresults structs */
#define UPROC_PROTRESULTS_INITIALIZER { NULL, 0, 0 }


/** Initialize a ::uproc_protresults struct */
void uproc_protresults_init(struct uproc_protresults *results);


/** Free allocated pointers of ::uproc_protresults struct */
void uproc_protresults_free(struct uproc_protresults *results);


/** Deep-copy a ::uproc_protresults struct */
int uproc_protresults_copy(struct uproc_protresults *dest,
                           const struct uproc_protresults *src);



/** \defgroup obj_protclass object uproc_protclass
 * Follows the \ref subsec_opaque
 * \{
 */

/** Protein sequence classifier object */
typedef struct uproc_protclass_s uproc_protclass;


typedef bool uproc_protfilter(const char*, size_t, uproc_family, double, void*);


/** Classification mode
 *
 * Determines which results ::uproc_protclass_classify produces.
 */
enum uproc_protclass_mode
{
    /** Only the result with the maximum score */
    UPROC_PROTCLASS_ALL,
    /** All results (unordered) */
    UPROC_PROTCLASS_MAX,
};

/** Create new protein classifier
 *
 * \param mode          Which results to produce
 * \param fwd           ::uproc_ecurve for looking up words in original order
 * \param rev           ::uproc_ecurve for looking up words in reversed order
 * \param substmat      Substitution matrix used to determine scores
 * \param filter        Result filtering function
 * \param filter_arg    Additional argument to \c filter
 */
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
/** \} */

/**
 * \}
 * \}
 */
#endif

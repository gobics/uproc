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

/** \file uproc/dnaclass.h
 *
 * \ref grp_clf_dna
 *
 * \weakgroup grp_clf
 * \{
 *
 * \weakgroup grp_clf_dna
 *
 * \details
 * Example usage:
 * \code
 * struct uproc_dnaresults results = UPROC_DNARESULTS_INITIALIZER;
 * uproc_dnaclass *dc = uproc_dc_create(...);
 * if (!dc) {
 *     // handle error
 * }
 * for (int i = 0; i < n; i++) {
 *     int res = uproc_dc_classify(dc, seqs[i], &results);
 *     if (res) {
 *         // handle error
 *     }
 *     // do something with the results
 * }
 * uproc_dnaresults_free(&results);
 * \endcode
 *
 * Related modules:
 * \li \ref grp_clf_prot
 * \li \ref grp_clf_orf
 *
 * \{
 */

#ifndef UPROC_DNACLASS_H
#define UPROC_DNACLASS_H

#include "uproc/orf.h"
#include "uproc/protclass.h"
#include "uproc/matrix.h"


/** DNA classification results
 *
* \defgroup struct_dnaresults struct uproc_dnaresults
* \{
 */
struct uproc_dnaresults
{
    struct uproc_dnapred
    {
        /** Predicted family */
        uproc_family family;
        /** Prediction score */
        double score;

        /** ORF from which the prediction was made */
        struct uproc_orf orf;
    } *preds;
    /** Number of results */
    size_t n;

    /** Allocated size of \c preds */
    size_t sz;
};


/** Initializer for ::uproc_dnaresults structs */
#define UPROC_DNARESULTS_INITIALIZER { NULL, 0, 0 }


/** Initialize a ::uproc_dnaresults struct */
void uproc_dnaresults_init(struct uproc_dnaresults *results);


/** Free allocated pointers of ::uproc_dnaresults struct */
void uproc_dnaresults_free(struct uproc_dnaresults *results);


/** Deep-copy a ::uproc_dnaresults struct */
int uproc_dnaresults_copy(struct uproc_dnaresults *dest,
                          const struct uproc_dnaresults *src);
/** \} */


/** DNA/RNA sequence classifier object
 *
 * \defgroup obj_dnaclass object uproc_dnaclass
 * \{
 */
typedef struct uproc_dnaclass_s uproc_dnaclass;


/** Classification mode
 *
 * Determines which results uproc_dc_create() produces.
 */
enum uproc_dnaclass_mode
{
    /** Only the result with the maximum score */
    UPROC_DNACLASS_ALL,

    /** All results (unordered) */
    UPROC_DNACLASS_MAX,
};


/** Create new DNA classifier
 *
 * \param mode              Which results to produce (see
 *                          ::uproc_dnaclass_mode)
 * \param pc                ::uproc_protclass to use for classifying ORFs
 * \param codon_scores      Codon scoring matrix (or NULL)
 * \param orf_filter        ORF filtering function (see orf.h)
 * \param orf_filter_arg    Additional argument to \c orf_filter
 */
uproc_dnaclass *uproc_dnaclass_create(enum uproc_dnaclass_mode mode,
                                      const uproc_protclass *pc,
                                      const uproc_matrix *codon_scores,
                                      uproc_orffilter *orf_filter,
                                      void *orf_filter_arg);


/** Destroy DNA classifier */
void uproc_dnaclass_destroy(uproc_dnaclass *dc);


/** Classify DNA sequence
 *
 * \c results should be a pointer to a struct ::uproc_dnaresults which has
 * been initialized to zero values (using ::UPROC_DNARESULTS_INITIALIZER or
 * with static storage duration) or which has already been used. It will
 * automatically be resized if needed.
 *
 * \param dc        DNA classifier
 * \param seq       sequence to classify
 * \param results   _OUT_: classification results
 */
int uproc_dnaclass_classify(const uproc_dnaclass *dc, const char *seq,
                            struct uproc_dnaresults *results);
/** \} */

/**
 * \}
 * \}
 */
#endif

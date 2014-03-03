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
 * See uproc_dc_classify() for a brief example.
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


/** Classification mode
 *
 * Determines which results uproc_dc_create() produces.
 */
enum uproc_dc_mode
{
    /** Only the result with the maximum score */
    UPROC_DC_ALL,

    /** All results (unordered) */
    UPROC_DC_MAX,
};


/** DNA classification results */
struct uproc_dc_results
{
    struct uproc_dc_pred
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


/** Initializer for ::uproc_dc_results objects */
#define UPROC_DC_RESULTS_INITIALIZER { NULL, 0, 0 }


/** \struct uproc_dnaclass
 *
 * Opaque DNA/RNA sequence classifier object
 */
typedef struct uproc_dnaclass_s uproc_dnaclass;


/** Create new DNA classifier
 *
 * \param mode              Which results to produce (see ::uproc_dc_mode)
 * \param pc                ::uproc_protclass to use for classifying ORFs
 * \param codon_scores      Codon scoring matrix (or NULL)
 * \param orf_filter        ORF filtering function (see orf.h)
 * \param orf_filter_arg    Additional argument to \c orf_filter
 */
uproc_dnaclass *uproc_dc_create(enum uproc_dc_mode mode,
                                const uproc_protclass *pc,
                                const uproc_matrix *codon_scores,
                                uproc_orf_filter *orf_filter,
                                void *orf_filter_arg);


/** Destroy DNA classifier */
void uproc_dc_destroy(uproc_dnaclass *dc);


/** Classify DNA sequence
 *
 * \c results should be a pointer to a struct ::uproc_dc_results which has
 * been initialized to zero values (using ::UPROC_DC_RESULTS_INITIALIZER or
 * with static storage duration) or which has already been used. It will
 * automatically be resized if needed.
 *
 * Example:
 * \code
 * struct uproc_dc_results results = UPROC_DC_RESULTS_INITIALIZER;
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
 * uproc_dc_results_free(&results);
 * \endcode
 *
 * \param dc        DNA classifier
 * \param seq       sequence to classify
 * \param results   _OUT_: classification results
 */
int uproc_dc_classify(const uproc_dnaclass *dc, const char *seq,
                      struct uproc_dc_results *results);

void uproc_dc_results_free(struct uproc_dc_results *results);

/**
 * \}
 * \}
 */
#endif

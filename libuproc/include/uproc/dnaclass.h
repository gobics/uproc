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
 * Module: \ref grp_clf_dna
 *
 * \weakgroup grp_clf
 * \{
 *
 * \weakgroup grp_clf_dna
 *
 * \details
 * Example usage:
 * \code
 * uproc_list *results = NULL;
 * uproc_dnaclass *dc = uproc_dc_create(...);
 * if (!dc) {
 *     // handle error
 * }
 * for (int i = 0; i < n; i++) {
 *     size_t j, n_results;
 *     struct uproc_dnaresult res;
 *     if (uproc_dc_classify(dc, seqs[i], &results)) {
 *         // handle error
 *     }
 *     n_results = uproc_list_size(results);
 *     for (j = 0; j < n_results; j++) {
 *         (void) uproc_list_get(results, j, &res);
 *          // do something with the result
 *     }
 * }
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

/** \defgroup struct_dnaresult struct uproc_dnaresult
 *
 * DNA classification result
 *
 * \{
 */

/** \copybrief struct_dnaresult */
struct uproc_dnaresult
{
    /** ORF from which the prediction was made */
    struct uproc_orf orf;

    /** Prediction for the ORF */
    struct uproc_protresult protresult;
};

/** Initializer for ::uproc_dnaresult structs */
#define UPROC_DNARESULT_INITIALIZER                         \
    {                                                       \
        UPROC_ORF_INITIALIZER, UPROC_PROTRESULT_INITIALIZER \
    }

/** Initialize a ::uproc_dnaresult struct */
void uproc_dnaresult_init(struct uproc_dnaresult *result);

/** Free allocated pointers of ::uproc_dnaresult struct */
void uproc_dnaresult_free(struct uproc_dnaresult *result);

/** Deep-copy a ::uproc_dnaresult struct */
int uproc_dnaresult_copy(struct uproc_dnaresult *dest,
                         const struct uproc_dnaresult *src);
/** \} */

/** \defgroup obj_dnaclass object uproc_dnaclass
 *
 * DNA/RNA sequence classifier
 *
 * An object of this type is used to classify DNA/RNA sequences. The result is
 * a \ref grp_datastructs_list of ::uproc_dnaresult objects. It does so in the
 * following way:
 *
 * \li A ::uproc_orfiter instance using the parameters that were passed to
 * uproc_dnaclass_create() is used to extract all relevant ORFs.
 *
 * \li Every ORF is classified with uproc_protclass_classify().
 *
 * \li For each protein class, the result of the best-scoring ORF is reported.
 *
 * \li If the ::UPROC_DNACLASS_MAX mode is used, only the protein class with
 * the highest score is retained in the result list.
 *
 * \{
 */

/** \struct uproc_dnaclass
 * \copybrief obj_dnaclass
 *
 * See \ref obj_dnaclass for details.
 */
typedef struct uproc_dnaclass_s uproc_dnaclass;

/** Classification mode
 *
 * Determines which results ::uproc_dnaclass_classify produces.
 */
enum uproc_dnaclass_mode {
    /** Only the result with the maximum score */
    UPROC_DNACLASS_ALL,

    /** All results (unordered) */
    UPROC_DNACLASS_MAX,
};

/** Create new DNA classifier
 *
 * \param mode              Which results to produce
 * \param pc                ::uproc_protclass to use for classifying ORFs
 * \param codon_scores      Codon scoring matrix (or NULL)
 * \param orf_filter        ORF filtering function
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
 * \c results should be a pointer to a \c (::uproc_list *) that is either NULL
 * (in which case a new list is created) or which has which has already been
 * used with this function.
 * The list will contain items of type \ref struct_dnaresult. If \c *results is
 * not NULL, all its elements will be passed to ::uproc_dnaresult_free at the
 * beginning.
 *
 * \param dc        DNA classifier
 * \param seq       sequence to classify
 * \param results   _OUT_: classification results
 */
int uproc_dnaclass_classify(const uproc_dnaclass *dc, const char *seq,
                            uproc_list **results);
/** \} */

/**
 * \}
 * \}
 */
#endif

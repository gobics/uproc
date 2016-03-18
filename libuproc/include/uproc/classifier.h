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

/** \file uproc/classifier.h
 *
 * Module: \ref grp_clf_clf
 *
 * \weakgroup grp_clf
 * \{
 *
 * \weakgroup grp_clf_clf
 *
 * \details
 * Example usage:
 * \code
 * uproc_list *results = NULL;
 * uproc_clf *clf = uproc_clf_create_dna(...);
 * if (!clf) {
 *     // handle error
 * }
 * for (int i = 0; i < n; i++) {
 *     size_t j, n_results;
 *     struct uproc_result res;
 *     if (uproc_clf_classify(clf, seqs[i], &results)) {
 *         // handle error
 *     }
 *     n_results = uproc_list_size(results);
 *     for (j = 0; j < n_results; j++) {
 *         (void) uproc_list_get(results, j, &res);
 *          // do something with the result
 *     }
 * }
 * uproc_clf_destroy(clf);
 * \endcode
 *
 * Related modules:
 * \li \ref grp_clf_dna
 * \li \ref grp_clf_prot
 * \li \ref grp_clf_orf
 *
 * \{
 */

#ifndef UPROC_CLASSIFIER_H
#define UPROC_CLASSIFIER_H

#include <stdbool.h>

#include "uproc/common.h"
#include "uproc/database.h"
#include "uproc/list.h"
#include "uproc/matrix.h"
#include "uproc/model.h"
#include "uproc/orf.h"

/** \defgroup struct_result struct uproc_result
 *
 * DNA classification result
 *
 * \{
 */

/** \copybrief struct_result */
struct uproc_result {
    /** The rank to which `class` belongs to */
    uproc_rank rank;

    /** Predicted class */
    uproc_class class;

    /** Prediction score */
    double score;

    /** List of \ref struct_mosaicword objects holding all matched
     * words for this family. Only set to non-NULL if the protein classifier
     * was used in "detailed" mode . */
    uproc_list *mosaicwords;

    /** If this is a result for a DNA/RAN sequence, the extracted ORF from
     * which the prediction was made */
    struct uproc_orf orf;
};

/** Initializer for ::uproc_result structs */
#define UPROC_RESULT_INITIALIZER      \
    {                                 \
        .orf = UPROC_ORF_INITIALIZER, \
    }

/** Initialize a ::uproc_result struct */
void uproc_result_init(struct uproc_result *result);

/** Free allocated pointers of ::uproc_result struct */
void uproc_result_free(struct uproc_result *result);

/** Free ::uproc_result struct, suitable for ::uproc_list_map2 */
void uproc_result_freev(void *ptr);

/** Deep-copy a ::uproc_result struct */
int uproc_result_copy(struct uproc_result *dest,
                      const struct uproc_result *src);

int uproc_result_cmp(const struct uproc_result *p1,
                     const struct uproc_result *p2);
/** \} */

/** \defgroup obj_clf object uproc_clf
 *
 * DNA/RNA sequence classifier
 *
 * An object of this type is used to classify DNA/RNA sequences. The result is
 * a \ref grp_datastructs_list of ::uproc_result objects. It does so in the
 * following way:
 *
 * \li A ::uproc_orfiter instance using the parameters that were passed to
 * uproc_clf_create() is used to extract all relevant ORFs.
 *
 * \li Every ORF is classified with uproc_protclass_classify().
 *
 * \li For each protein class, the result of the best-scoring ORF is reported.
 *
 * \li If the ::UPROC_CLF_MAX mode is used, only the protein class with
 * the highest score is retained in the result list.
 *
 * \{
 */

/** \struct uproc_clf
 * \copybrief obj_clf
 *
 * See \ref obj_clf for details.
 */
typedef struct uproc_clf_s uproc_clf;

/** Classification mode
 *
 * Determines which results ::uproc_clf_classify produces.
 */
enum uproc_clf_mode {
    /** All results (unordered) */
    UPROC_CLF_ALL,

    /** Only the result with the maximum score */
    UPROC_CLF_MAX,
};

/** Destroy classifier */
void uproc_clf_destroy(uproc_clf *clf);

/** Classify sequence
 *
 * \c results should be a pointer to a \c (::uproc_list *) that is either NULL
 * (in which case a new list is created) or which has which has already been
 * used with this function.
 * The list will contain items of type \ref struct_result. If \c *results is
 * not NULL, all its elements will be passed to ::uproc_result_free at the
 * beginning.
 *
 * \param clf       classifier
 * \param seq       sequence to classify
 * \param results   _OUT_: classification results
 */
int uproc_clf_classify(const uproc_clf *clf, const char *seq,
                       uproc_list **results);

/** Protein filter function type
 *
 * Used by uproc_protclass_classify() to decide whether a classification should
 * be added to the results.
 *
 * \param seq       classified sequence
 * \param seq_len   length of the classified sequence
 * \param class     predicted class
 * \param score     total score for this class
 * \param arg       user-supplied argument
 *
 */
typedef bool uproc_protfilter(const char *seq, size_t seq_len,
                              uproc_class class, double score, void *arg);

uproc_clf *uproc_clf_create_protein(enum uproc_clf_mode mode, bool detailed,
                                    const uproc_ecurve *fwd,
                                    const uproc_ecurve *rev,
                                    const uproc_substmat *substmat,
                                    uproc_protfilter *filter, void *filter_arg);

uproc_clf *uproc_clf_create_dna(enum uproc_clf_mode mode, uproc_clf *protclass,
                                const uproc_matrix *codon_scores,
                                uproc_orffilter *orf_filter,
                                void *orf_filter_arg);

/** \} */

/**
 * \}
 * \}
 */
#endif

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
 * Module: \ref grp_clf_prot
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
#include "uproc/list.h"

/** \defgroup struct_matchedword struct uproc_matchedword
 * Word that was matched in an ecurve.
 * \{
 */

struct uproc_matchedword
{
    /** The matched aminoacid word. */
    struct uproc_word word;

    /** Position in the protein sequence. */
    size_t index;

    /** Whether this word matched in the forward or reverse ecurve */
    bool reverse;

    /** Sum of the scores of all positions of this word, independent of whether
     * they contribute to the mosaic score or not.
     *
     * TODO: is this the desired way of computing the score? */
    double score;
};

#define UPROC_MATCHEDWORD_INITIALIZER         \
    {                                         \
        UPROC_WORD_INITIALIZER, 0, false, 0.0 \
    }

/** Initialize a ::uproc_matchedword struct */
void uproc_matchedword_init(struct uproc_matchedword *results);

/** Free allocated pointers of a ::uproc_matchedword struct */
void uproc_matchedword_free(struct uproc_matchedword *results);

/** Deep-copy a ::uproc_matchedword struct */
int uproc_matchedword_copy(struct uproc_matchedword *dest,
                           const struct uproc_matchedword *src);

/** \} */

/** \defgroup struct_protresult struct uproc_protresult
 * Protein classification result
 * \{
 */

/** \copybrief struct_protresult */
struct uproc_protresult
{
    /** Predicted family */
    uproc_family family;

    /** Prediction score */
    double score;

    /** List of \ref struct_matchedword objects holding all matched
     * words for this family. Only set to non-NULL if the protein classifier
     * was used in "detailed" mode . */
    uproc_list *matched_words;
};

/** Initializer for ::uproc_protresult structs */
#define UPROC_PROTRESULT_INITIALIZER \
    {                                \
        0, 0, NULL                   \
    }

/** Initialize a ::uproc_protresult struct */
void uproc_protresult_init(struct uproc_protresult *results);

/** Free allocated pointers of a ::uproc_protresult struct */
void uproc_protresult_free(struct uproc_protresult *results);

/** Deep-copy a ::uproc_protresult struct */
int uproc_protresult_copy(struct uproc_protresult *dest,
                          const struct uproc_protresult *src);
/** \} */

/** \defgroup obj_protclass object uproc_protclass
 *
 * Protein sequence classifier
 *
 * \{
 */

/** \struct uproc_protclass
 * \copybrief obj_protclass
 *
 * See \ref obj_protclass for details.
 *
 */
typedef struct uproc_protclass_s uproc_protclass;

/** Protein filter function type
 *
 * Used by uproc_protclass_classify() to decide whether a classification should
 * be added to the results.
 *
 * \param seq       classified sequence
 * \param seq_len   length of the classified sequence
 * \param family    predicted family
 * \param score     total score for this family
 * \param arg       user-supplied argument
 *
 */
typedef bool uproc_protfilter(const char *seq, size_t seq_len,
                              uproc_family family, double score, void *arg);

/** Classification mode
 *
 * Determines which results uproc_protclass_classify() produces.
 */
enum uproc_protclass_mode {
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
                                        bool detailed,
                                        const uproc_ecurve *fwd,
                                        const uproc_ecurve *rev,
                                        const uproc_substmat *substmat,
                                        uproc_protfilter *filter,
                                        void *filter_arg);

/** Destroy protein classifier */
void uproc_protclass_destroy(uproc_protclass *pc);

/** Classify DNA sequence
 *
 * \c results should be a pointer to a \c (::uproc_list *) that is either NULL
 * (in which case a new list is created) or which has which has already been
 * used with this function.
 * The list will contain items of type \ref struct_protresult. If \c *results
 * is not NULL, all its elements will be passed to uproc_protresult_free() in
 * the beginning.
 *
 * \param pc        protein classifier
 * \param seq       sequence to classify
 * \param results   _OUT_: classification results
 */
int uproc_protclass_classify(const uproc_protclass *pc, const char *seq,
                             uproc_list **results);

/** Tracing callback type
 *
 * Additionally to the normal classification, it's possible to get information
 * about every matched word by installing a tracing callback function with
 * uproc_protclass_trace_cb().
 *
 * This function will be called for every word match that was found in the
 * ecurves while classifying a protein sequence.
 *
 * \param word      the word found in the ecurve
 * \param family    the corresponding family entry
 * \param index     position in the protein sequence
 * \param reverse   whether the word was found in the "reverse" ecurve
 * \param scores    scores of this match (array of size ::UPROC_SUFFIX_LEN)
 * \param arg       user-supplied argument
 */
typedef void uproc_protclass_trace_cb(const struct uproc_word *word,
                                      uproc_family family, size_t index,
                                      bool reverse, const double *scores,
                                      void *opaque);

/** Set trace callback
 *
 * \param pc        protein classifier
 * \param cb        callback function
 * \param cb_arg    additional argument to \c cb
 */
void uproc_protclass_set_trace(uproc_protclass *pc,
                               uproc_protclass_trace_cb *cb, void *cb_arg);
/** \} */

/**
 * \}
 * \}
 */
#endif

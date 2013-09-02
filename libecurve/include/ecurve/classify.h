#ifndef EC_CLASSIFY_H
#define EC_CLASSIFY_H

/** \file ecurve/classify.h
 *
 * Classify amino acid sequences
 */

#include "ecurve/common.h"
#include "ecurve/substmat.h"
#include "ecurve/ecurve.h"
#include "ecurve/matrix.h"
#include "ecurve/orf.h"


/** Classify amino acid sequence
 *
 * The following algorithm is used:
 *
 *   -  For all words in the sequence `seq`:
 *       -# Look up forward input word in forward ecurve.
 *       -# Align suffix of input and found word(s) using position-sentive
 *          alignment using the distance matrices in `substmat`.
 *       -# Insert alignment distances into the score vector associated with the
 *          found protein class.
 *       -# Repeat for reverse word and ecurve.
 *
 *   -  Sum up score vectors of all protein classes and find the class with
 *      maximum score.
 *
 *  The score vector of a protein class (virtually) holds the highest alignment
 *  score obtained at a given position.
 *
 * _One_ of `fwd_ecurve` and `rev_ecurve` may be a null pointer to "ignore"
 * that direction.
 * If both are non-null, they should use the same alphabet.
 *
 * Yields all classifications (unsorted).
 *
 * \param seq           amino acid sequence
 * \param substmat       distance matrices for aligning the suffixes
 * \param fwd_ecurve    ecurve for looking up forward words
 * \param rev_ecurve    ecurve for looking up reverse words
 * \param predict_count _OUT_: number of predictions
 * \param predict_cls   _OUT_: protein classes
 * \param predict_score _OUT_: scores
 *
 * \retval #EC_SUCCESS  success
 * \retval #EC_ENOMEM   memory allocation failed
 */
int ec_classify_protein_all(
        const char *seq,
        const struct ec_substmat substmat[static EC_SUFFIX_LEN],
        const struct ec_ecurve *fwd_ecurve,
        const struct ec_ecurve *rev_ecurve,
        size_t *predict_count,
        ec_class **predict_cls,
        double **predict_score);

/** Like ec_classify_protein_all(), but yielding only the prediction with
 * maximum score.
 */
int ec_classify_protein_max(
        const char *seq,
        const struct ec_substmat substmat[static EC_SUFFIX_LEN],
        const struct ec_ecurve *fwd_ecurve,
        const struct ec_ecurve *rev_ecurve,
        ec_class *predict_cls,
        double *predict_score);


/** Classify DNA/RNA sequence
 *
 * Translates DNA/RNA sequence using ecurve/orf.h and classifies them using
 * ec_classify_protein_all(). Depending on the `mode` argument, frames/strands
 * are scored separately, `predict_cls`, `predict_score` and `orf_lengths`
 * should point into arrays of sufficient size.
 */
int ec_classify_dna_all(
        const char *seq,
        enum ec_orf_mode mode,
        const struct ec_orf_codonscores *codon_scores,
        const struct ec_matrix *thresholds,
        const struct ec_substmat substmat[static EC_SUFFIX_LEN],
        const struct ec_ecurve *fwd_ecurve,
        const struct ec_ecurve *rev_ecurve,
        size_t *predict_count,
        ec_class **predict_cls,
        double **predict_score,
        size_t *orf_lengths);

/** Like ec_classify_dna_all(), but yielding only the prediction with maximum
 * score.
 */
int ec_classify_dna_max(
        const char *seq,
        enum ec_orf_mode mode,
        const struct ec_orf_codonscores *codon_scores,
        const struct ec_matrix *thresholds,
        const struct ec_substmat substmat[static EC_SUFFIX_LEN],
        const struct ec_ecurve *fwd_ecurve,
        const struct ec_ecurve *rev_ecurve,
        ec_class *predict_cls,
        double *predict_score,
        size_t *orf_lengths);

#endif

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

/** \file uproc/orf.h
 *
 * Module: \ref grp_clf_orf
 *
 * \ref grp_clf_orf
 *
 * \weakgroup grp_clf
 * \{
 * \weakgroup grp_clf_orf
 * \{
 */

#ifndef UPROC_ORF_H
#define UPROC_ORF_H

#include "uproc/common.h"
#include "uproc/io.h"
#include "uproc/matrix.h"


/** Number of possible frames (forward and reverse) */
#define UPROC_ORF_FRAMES 6


/** \defgroup struct_orf struct uproc_orf
 * \details
 * See also
 * \li \ref subsec_struct
 * \{
 */

/** Open reading frame
 *
 * The (partial) result of translating DNA into protein. An ORF extracted by
 * ::uproc_orfiter_next ends after one of the three stop codons (TAA, TAG, TGA)
 * and starts either at the beginning of the sequence or after the stop codon
 * that terminated the previous ORF.
 *
 */
struct uproc_orf
{
    /** Derived amino acid sequence as string */
    char *data;

    /** Starting index w.r.t. the original DNA string */
    size_t start;

    /** Length of the amino acid sequence */
    size_t length;

    /** Sum of codon scores */
    double score;

    /** On which frame the ORF was found */
    unsigned frame;
};


/** Initializer macro */
#define UPROC_ORF_INITIALIZER { NULL, 0, 0, 0.0, 0 }


/** Initializer function */
void uproc_orf_init(struct uproc_orf *orf);


/** Freeing function */
void uproc_orf_free(struct uproc_orf *orf);

/** Deep-copy function */
int uproc_orf_copy(struct uproc_orf *dest, const struct uproc_orf *src);
/** \} */


/** ORF filter function
 *
 * The function should take an ORF, the DNA sequence, it's length and GC
 * content and finally a user-supplied "opaque" pointer as arguments and return
 * whether the ORF is accepted or not.
 */
typedef bool uproc_orffilter(const struct uproc_orf*, const char *, size_t,
                             double, void*);


/** Prepare codon score table
 *
 * Turns a ::uproc_matrix of size <tt>::UPROC_CODON_COUNT x 1</tt> into a
 * double array suitable for uproc_orfiter_create(). The score of a codon
 * containing wildcards is the mean value of all codons that match it. Stop
 * codons don't get a score (it is ignored anyway)
 *
 * If \c score_matrix is NULL, all entries of \c scores are set to 0.
 *
 * (This is a quite costly operation, so doing this once instead of every time
 * a ::uproc_orfiter is created can save a lot of time.)
 *
 * \param scores        _OUT_: scores for all possible binary codons
 *                      (must be a pointer into an array of at least
 *                      ::UPROC_BINARY_CODON_COUNT elements)
 * \param score_matrix  codon scores, size <tt>::UPROC_CODON_COUNT x 1</tt>
 */
void uproc_orf_codonscores(double *scores, const uproc_matrix *score_matrix);


/** \defgroup obj_orfiter object uproc_orfiter
 *
 * \details
 * See also:
 *
 * \li \ref subsec_opaque
 * \li \ref subsec_opaque_iter
 * \{
 */

/** Iterates over a DNA/RNA sequence and yield all possible ORFs */
typedef struct uproc_orfiter_s uproc_orfiter;

/** Create orfiter object
 *
 * \param seq           sequence to iterate over
 * \param codon_scores  codon scores, must be a pointer to the first element of
 *                      an array of size ::UPROC_BINARY_CODON_COUNT (see also
 *                      uproc_orf_codonscores())
 * \param filter        filter function
 * \param filter_arg    additional argument to \c filter
 *
 */
uproc_orfiter *uproc_orfiter_create(
    const char *seq,
    const double *codon_scores,
    uproc_orffilter *filter, void *filter_arg);

/** Destroy orfiter object */
void uproc_orfiter_destroy(uproc_orfiter *iter);


/** Obtain the next ORF
 *
 * A _shallow_ copy of the next ORF will be stored in \c *orf, if you need to
 * store a copy, use uproc_orf_copy().
 *
 * \param iter  orfiter instance
 * \param next  _OUT_: read ORF
 */
int uproc_orfiter_next(uproc_orfiter *iter, struct uproc_orf *next);
/** \} */

/**
 * \}
 * \}
 */
#endif

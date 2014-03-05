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

/** Prepare codon score table */
void uproc_orf_codonscores(double *scores, const uproc_matrix *score_matrix);

/** \defgroup obj_orfiter object uproc_orfiter
 * \{
 */

/** Iterates over a DNA/RNA sequence and yield all possible ORFs */
typedef struct uproc_orfiter_s uproc_orfiter;

/** Initialize ORF iterator
 *
 * \param iter          _OUT_: iterator object
 * \param seq           sequence to iterate over
 * \param codon_scores  codon scores, must be a pointer to the first element of
 *                      an array of size #UPROC_BINARY_CODON_COUNT
 */
uproc_orfiter *uproc_orfiter_create(
    const char *seq,
    const double *codon_scores,
    uproc_orffilter *filter, void *filter_arg);

/** Free memory of an ORF iterator */
void uproc_orfiter_destroy(uproc_orfiter *iter);

/** Obtain the next ORF from an iterator
 *
 * A _shallow_ copy of the next ORF will be stored in `*orf`.
 * Don't free() any members and don't use it after calling
 * uproc_orfiter_destroy().
 *
 * \param iter  ORF iterator
 * \param next  _OUT_: read ORF
 *
 * \retval #UPROC_ITER_YIELD   an ORF was read successfully
 * \retval #UPROC_ITER_STOP    the end of the sequence was reached
 * \retval other            an error occured
 */
int uproc_orfiter_next(uproc_orfiter *iter, struct uproc_orf *next);
/** \} */

/**
 * \{
 * \{
 */
#endif

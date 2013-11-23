#ifndef UPROC_ORF_H
#define UPROC_ORF_H

/** \file uproc/orf.h
 *
 * Extract open reading frames from DNA/RNA sequences
 */

#include "uproc/common.h"
#include "uproc/io.h"
#include "uproc/matrix.h"

/** Number of possible frames (forward and reverse) */
#define UPROC_ORF_FRAMES 6

/** Character to separate the results of uproc_orf_chained() */
#define UPROC_ORF_SEPARATOR '*'

/** ORF */
struct uproc_orf
{
    /** Derived amino acid sequence as string */
    char *data;

    /** Length of the amino acid sequence */
    size_t length;

    /** Sum of codon scores */
    double score;

    /** On which frame the ORF was found */
    unsigned frame;
};

/** ORF filter function
 *
 * The function should take an ORF, the DNA sequence, it's length and GC
 * content and finally a user-supplied "opaque" pointer as arguments and return
 * whether the ORF is accepted or not.
 */
typedef bool uproc_orf_filter(const struct uproc_orf*, const char *, size_t, double,
        void*);

/** Iterates over a DNA/RNA sequence and yield all possible ORFs */
struct uproc_orfiter
{
    /** DNA/RNA sequence being iterated */
    const char *seq;

    /** Length of the sequence */
    size_t seq_len;

    /** GC content of the sequence */
    double seq_gc;

    /** User-supplied argument to `filter` */
    void *filter_arg;

    /** Pointer to filter function */
    uproc_orf_filter *filter;

    /** current position in the DNA/RNA sequence */
    const char *pos;

    /** Codon score table */
    const double *codon_scores;

    /** Number of processed nucleotide symbols */
    size_t nt_count;

    /** Current frame */
    unsigned frame;

    /** Current forward codons */
    uproc_codon codon[UPROC_ORF_FRAMES / 2];

    /** ORFs to "work with" */
    struct uproc_orf orf[UPROC_ORF_FRAMES];

    /** Sizes of the corresponding orf.data buffers */
    size_t data_sz[UPROC_ORF_FRAMES];

    /** Indicate whether an ORF was completed and should be returned next */
    bool yield[UPROC_ORF_FRAMES];
};

/** Prepare codon score table */
void uproc_orf_codonscores(double scores[static UPROC_BINARY_CODON_COUNT],
        const struct uproc_matrix *score_matrix);

/** Initialize ORF iterator
 *
 * \param iter          _OUT_: iterator object
 * \param seq           sequence to iterate over
 * \param codon_scores  codon scores, must be a pointer into an array of size
 *                      #UPROC_BINARY_CODON_COUNT
 */
int uproc_orfiter_init(struct uproc_orfiter *iter, const char *seq,
        const double codon_scores[static UPROC_BINARY_CODON_COUNT],
        uproc_orf_filter *filter, void *filter_arg);

/** Free memory of an ORF iterator */
void uproc_orfiter_destroy(struct uproc_orfiter *iter);

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
int uproc_orfiter_next(struct uproc_orfiter *iter, struct uproc_orf *next);

#endif

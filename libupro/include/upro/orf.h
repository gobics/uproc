#ifndef UPRO_ORF_H
#define UPRO_ORF_H

/** \file upro/orf.h
 *
 * Extract open reading frames from DNA/RNA sequences
 */

#include "upro/common.h"
#include "upro/io.h"
#include "upro/matrix.h"

/** Number of possible frames (forward and reverse) */
#define UPRO_ORF_FRAMES 6

/** Character to separate the results of upro_orf_chained() */
#define UPRO_ORF_SEPARATOR '*'

/** ORF */
struct upro_orf
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
typedef bool upro_orf_filter(const struct upro_orf*, const char *, size_t, double,
        void*);

/** Iterates over a DNA/RNA sequence and yield all possible ORFs */
struct upro_orfiter
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
    upro_orf_filter *filter;

    /** current position in the DNA/RNA sequence */
    const char *pos;

    /** Codon score table */
    const double *codon_scores;

    /** Number of processed nucleotide symbols */
    size_t nt_count;

    /** Current frame */
    unsigned frame;

    /** Current forward codons */
    upro_codon codon[UPRO_ORF_FRAMES / 2];

    /** ORFs to "work with" */
    struct upro_orf orf[UPRO_ORF_FRAMES];

    /** Sizes of the corresponding orf.data buffers */
    size_t data_sz[UPRO_ORF_FRAMES];

    /** Indicate whether an ORF was completed and should be returned next */
    bool yield[UPRO_ORF_FRAMES];
};

/** Prepare codon score table */
void upro_orf_codonscores(double scores[static UPRO_BINARY_CODON_COUNT],
        const struct upro_matrix *score_matrix);

/** Initialize ORF iterator
 *
 * \param iter          _OUT_: iterator object
 * \param seq           sequence to iterate over
 * \param codon_scores  codon scores, must be a pointer into an array of size
 *                      #UPRO_BINARY_CODON_COUNT
 */
int upro_orfiter_init(struct upro_orfiter *iter, const char *seq,
        const double codon_scores[static UPRO_BINARY_CODON_COUNT],
        upro_orf_filter *filter, void *filter_arg);

/** Free memory of an ORF iterator */
void upro_orfiter_destroy(struct upro_orfiter *iter);

/** Obtain the next ORF from an iterator
 *
 * A _shallow_ copy of the next ORF will be stored in `*orf`.
 * Don't free() any members and don't use it after calling
 * upro_orfiter_destroy().
 *
 * \param iter  ORF iterator
 * \param next  _OUT_: read ORF
 *
 * \retval #UPRO_ITER_YIELD   an ORF was read successfully
 * \retval #UPRO_ITER_STOP    the end of the sequence was reached
 * \retval other            an error occured
 */
int upro_orfiter_next(struct upro_orfiter *iter, struct upro_orf *next);

#endif

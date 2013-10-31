#ifndef EC_ORF_H
#define EC_ORF_H

/** \file ecurve/orf.h
 *
 * Extract open reading frames from DNA/RNA sequences
 */

#include "ecurve/common.h"
#include "ecurve/io.h"
#include "ecurve/matrix.h"

/** Number of possible frames (forward and reverse) */
#define EC_ORF_FRAMES 6

/** Character to separate the results of ec_orf_chained() */
#define EC_ORF_SEPARATOR '*'

/** ORF */
struct ec_orf
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

/** Codon score table */
struct ec_orf_codonscores
{
    double values[EC_BINARY_CODON_COUNT];
};


/** ORF filter function
 *
 * The function should take an ORF, the DNA sequence, it's length and GC
 * content and finally a user-supplied "opaque" pointer as arguments and return
 * whether the ORF is accepted or not.
 */
typedef bool ec_orf_filter(const struct ec_orf*, const char *, size_t, double,
        void*);


/** Iterates over a DNA/RNA sequence and yield all possible ORFs */
struct ec_orfiter
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
    ec_orf_filter *filter;

    /** current position in the DNA/RNA sequence */
    const char *pos;

    /** Codon score table */
    const struct ec_orf_codonscores *codon_scores;

    /** Number of processed nucleotide symbols */
    size_t nt_count;

    /** Current frame */
    unsigned frame;

    /** Current forward codons */
    ec_codon codon[EC_ORF_FRAMES / 2];

    /** ORFs to "work with" */
    struct ec_orf orf[EC_ORF_FRAMES];

    /** Sizes of the corresponding orf.data buffers */
    size_t data_sz[EC_ORF_FRAMES];

    /** Indicate whether an ORF was completed and should be returned next */
    bool yield[EC_ORF_FRAMES];
};


/** Load codon scores from file */
int ec_orf_codonscores_load_file(struct ec_orf_codonscores *scores,
        const char *path, enum ec_io_type iotype);

/** Load codon scores from stream */
int ec_orf_codonscores_load_stream(struct ec_orf_codonscores *scores,
        ec_io_stream *stream);

/** Initialize codon scores from a matrix
 *
 * \param scores        score table to initialize
 * \param score_matrix  a #EC_CODON_COUNT x 1 matrix containing the scores for
 *                      each "real" codon
 */
void ec_orf_codonscores_init(struct ec_orf_codonscores *scores,
        const struct ec_matrix *score_matrix);

/** Initialize ORF iterator
 *
 * \param iter          _OUT_: iterator object
 * \param seq           sequence to iterate over
 * \param codon_scores  codon scores, must be a #EC_CODON_COUNT x 1 matrix
 *
 */
int ec_orfiter_init(struct ec_orfiter *iter, const char *seq,
        const struct ec_orf_codonscores *codon_scores,
        ec_orf_filter *filter, void *filter_arg);

/** Free memory of an ORF iterator */
void ec_orfiter_destroy(struct ec_orfiter *iter);

/** Obtain the next ORF from an iterator
 *
 * A _shallow_ copy of the next ORF will be stored in `*orf`.
 * Don't free() any members and don't use it after calling
 * ec_orfiter_destroy().
 *
 * \param iter  ORF iterator
 * \param next  _OUT_: read ORF
 *
 * \retval #EC_ITER_YIELD   an ORF was read successfully
 * \retval #EC_ITER_STOP    the end of the sequence was reached
 * \retval other            an error occured
 */
int ec_orfiter_next(struct ec_orfiter *iter, struct ec_orf *next);

#endif

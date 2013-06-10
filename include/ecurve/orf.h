#ifndef EC_ORF_H
#define EC_ORF_H

/** \file ecurve/orf.h
 *
 * Extract open reading frames from DNA/RNA sequences
 */

#include "ecurve/common.h"

/** Number of possible frames (forward and reverse) */
#define EC_ORF_FRAMES 6

/** Character to separate the results of ec_orf_chained() */
#define EC_ORF_SEPARATOR '*'

struct ec_orf {
    /** Derived amino acid sequence */
    char *data;

    /** Length of the amino acid sequence */
    size_t length;

    /** Sum of codon scores */
    double score;
};

/** Type to iterate over a DNA/RNA sequence and yield all possible ORFs */
typedef struct ec_orfiter_s ec_orfiter;

/** ORF iterator struct */
struct ec_orfiter_s {
    /** current position in the DNA/RNA sequence */
    const char *pos;

    /** Codon score table
     *
     * Calculated from the third argument to ec_orfiter_init() such that
     * the score of a codon mask is the mean of all codons it matches
     */
    double codon_scores[EC_BINARY_CODON_COUNT];

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

/** How to deal with ORFs from different frames */
enum ec_orf_mode {
    /** Treat all frames equally */
    EC_ORF_ALL = 1,

    /** Differentiate between forward and reverse strand */
    EC_ORF_PER_STRAND = 2,

    /** Treat eall frames uniquely */
    EC_ORF_PER_FRAME = EC_ORF_FRAMES,
};

/** Initialize ORF iterator
 *
 * \param iter          _OUT_: iterator object
 * \param seq           sequence to iterate over
 * \param codon_scores  codon score vector
 *
 */
int ec_orfiter_init(ec_orfiter *iter, const char *seq,
                    const double codon_scores[static EC_CODON_COUNT]);

/** Free memory of an ORF iterator */
void ec_orfiter_destroy(ec_orfiter *iter);

/** Obtain the next ORF from an iterator
 *
 * \param iter  ORF iterator
 * \param orf   _OUT_: read ORF
 * \param frame _OUT_: from which frame the ORF was obtained
 *
 * \retval EC_SUCCESS       an ORF was read successfully
 * \retval EC_FAILURE       an error occured
 * \retval EC_ITER_STOP     the end of the sequence was reached
 */
int ec_orfiter_next(ec_orfiter *iter, struct ec_orf *orf, unsigned *frame);

/** Extract ORFs from a DNA/RNA sequence
 *
 * Concatenates all ORFs of length >= `min_length` and sum of codon scores >=
 * `min_score` separated by #EC_ORF_SEPARATOR.
 *
 * For each n between 0 and `mode - 1`, the buffer `buf[n]` must be a null
 * pointer or pointer to allocated storage of `sz[n]` bytes. If `buf[n]` is too
 * small, it will be resized using realloc() and `sz[n]` updated accordingly.
 * This mimics the behaviour of POSIX' `getline()`
 * (http://pubs.opengroup.org/onlinepubs/9699919799/functions/getline.html)
 *
 * \param seq           DNA/RNA sequence
 * \param mode          how different frames should be treated
 * \param codon_scores  codon score table
 * \param min_score     minimum codon score sum
 * \param min_length    minimum ORF length
 * \param buf           buffer(s) for output strings
 * \param sz            sizes of output buffers
 */
int ec_orf_chained(const char *seq,
                   enum ec_orf_mode mode,
                   const double codon_scores[static EC_CODON_COUNT],
                   double min_score,
                   size_t min_length,
                   char **buf,
                   size_t *sz);

#endif

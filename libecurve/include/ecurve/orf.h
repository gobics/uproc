#ifndef EC_ORF_H
#define EC_ORF_H

/** \file ecurve/orf.h
 *
 * Extract open reading frames from DNA/RNA sequences
 */

#include "ecurve/common.h"
#include "ecurve/matrix.h"

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

/** Codon score table */
struct ec_orf_codonscores {
    double values[EC_BINARY_CODON_COUNT];
};

/** Iterates over a DNA/RNA sequence and yield all possible ORFs */
struct ec_orfiter {
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

/** How to deal with ORFs from different frames */
enum ec_orf_mode {
    /** Treat all frames equally */
    EC_ORF_ALL = 1,

    /** Differentiate between forward and reverse strand */
    EC_ORF_PER_STRAND = 2,

    /** Treat eall frames uniquely */
    EC_ORF_PER_FRAME = EC_ORF_FRAMES,
};

/** Load codon scores from file */
int ec_orf_codonscores_load_file(struct ec_orf_codonscores *scores,
                                 const char *path);

/** Load codon scores from stream */
int ec_orf_codonscores_load_stream(struct ec_orf_codonscores *scores,
                                   gzFile stream);

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
                    const struct ec_orf_codonscores *codon_scores);

/** Free memory of an ORF iterator */
void ec_orfiter_destroy(struct ec_orfiter *iter);

/** Obtain the next ORF from an iterator
 *
 * \param iter  ORF iterator
 * \param orf   _OUT_: read ORF
 * \param frame _OUT_: from which frame the ORF was obtained
 *
 * \retval #EC_ITER_YIELD   an ORF was read successfully
 * \retval #EC_ITER_STOP    the end of the sequence was reached
 * \retval other            an error occured
 */
int ec_orfiter_next(struct ec_orfiter *iter, struct ec_orf *orf, unsigned *frame);

/** Extract ORFs from a DNA/RNA sequence
 *
 * Concatenates all ORFs of length >= #EC_WORD_LEN and a minimum score obtained
 * from `thresholds`, depending on the GC content and sequence length.
 * The score of an ORF is obtained by summing up the entries of `codon_scores`
 * for each codon in the sequence.
 *
 * If `codon_scores` is a null pointer, each codon gets a zero score, if
 * `thresholds` is a null pointer, every ORF that is long enough will be
 * accepted.
 *
 * The result is a string with all those ORFs, separated by #EC_ORF_SEPARATOR.
 *
 * For each n between 0 and `mode - 1`, the buffer `buf[n]` must be a null
 * pointer or pointer to allocated storage of `sz[n]` bytes. If `buf[n]` is too
 * small, it will be resized using realloc() and `sz[n]` updated accordingly.
 * This mimics the behaviour of POSIX' `getline()`
 * (http://pubs.opengroup.org/onlinepubs/9699919799/functions/getline.html)
 *
 * \param seq           DNA/RNA sequence
 * \param mode          how different frames should be treated
 * \param codon_scores  codon scores
 * \param thresholds    score threshold matrix
 * \param buf           buffer(s) for output strings
 * \param sz            sizes of output buffers
 */
int ec_orf_chained(const char *seq,
                   enum ec_orf_mode mode,
                   const struct ec_orf_codonscores *codon_scores,
                   const struct ec_matrix *thresholds,
                   char **buf,
                   size_t *sz);

#endif

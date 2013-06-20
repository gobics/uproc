#ifndef EC_SEQIO_H
#define EC_SEQIO_H

/** \file ecurve/seqio.h
 *
 * Sequence input/output
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

/** Modes to treat input sequence characters */
enum ec_seqio_filter_mode {
    /** Store the character */
    EC_SEQIO_ACCEPT,
    /** Don't store the character and increment the ignore count */
    EC_SEQIO_IGNORE,
    /** Store the character, but increment the warning count */
    EC_SEQIO_WARN,
};

/** Flags controlling the behaviour of a sequence filter */
enum ec_seqio_filter_flags {
    /** Reset ignore and warning counts to zero before reading a sequence */
    EC_SEQIO_F_RESET = (1 << 0),
};

/** Struct defining a sequence filter */
struct ec_seqio_filter {
    /** How to treat each character */
    enum ec_seqio_filter_mode mode[1 << CHAR_BIT];

    /** Flags controlling the mode of operation */
    enum ec_seqio_filter_flags flags;

    /** Number of ignored characters */
    unsigned ignored;

    /** Number of warnings */
    unsigned warnings;
};

/** Initialize a sequence filter
 *
 * \param filter        filter to initialize
 * \param flags         bitwise-OR combination of flags to use
 * \param default_mode  behaviour for characters present in neither of the
 *                      following arguments
 * \param accept        characters to accept
 * \param ignore        characters to ignore
 * \param warn          characters to warn about
 */
void ec_seqio_filter_init(struct ec_seqio_filter *filter, 
                          enum ec_seqio_filter_flags flags,
                          enum ec_seqio_filter_mode default_mode,
                          const char *accept, const char *ignore,
                          const char *warn);

/** Read FASTA formatted sequence from stream
 *
 * When reading the sequence data, whitespace characters are ignored silently
 * and if `filter` is not a null pointer, it will be applied to anything else.
 *
 * The `char** / size_t*` pairs (`id / id_size` etc.) behave like `lineptr / n`
 * in POSIX' `getline()`
 * (see http://pubs.opengroup.org/onlinepubs/9699919799/functions/getline.html).
 */
int ec_seqio_fasta_read(FILE *stream, struct ec_seqio_filter *filter,
                  char **id, size_t *id_size,
                  char **comment, size_t *comment_size,
                  char **seq, size_t *seq_size);

/** Print FASTA formatted sequence to stream
 *
 * \param stream    stream to write to
 * \param id        sequence id
 * \param comment   comment(s) or null pointer
 * \param seq       sequence data
 * \param width     how many sequence characters are printed per line, if the
 *                  value 0 is specified, no line breaks are added
 */
void ec_seqio_fasta_print(FILE *stream, const char *id, const char *comment,
                    const char *seq, unsigned width);
#endif

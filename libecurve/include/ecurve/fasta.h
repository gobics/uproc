#ifndef EC_FASTA_H
#define EC_FASTA_H

/** \file ecurve/fasta.h
 *
 * Sequence input/output
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

struct ec_fasta_reader
{
    char *line;
    long line_len;
    size_t line_sz, line_no;
    char *header, *comment, *seq;
    size_t header_len, comment_len, seq_len;
    size_t header_sz, comment_sz, seq_sz, seq_sz_hint;
};

void ec_fasta_reader_init(struct ec_fasta_reader *rd, size_t seq_sz_hint);

void ec_fasta_reader_free(struct ec_fasta_reader *rd);

int ec_fasta_read(FILE *stream, struct ec_fasta_reader *rd);


/** Print FASTA formatted sequence to stream
 *
 * \param stream    stream to write to
 * \param id        sequence id
 * \param comment   comment(s) or null pointer
 * \param seq       sequence data
 * \param width     how many sequence characters are printed per line, if the
 *                  value 0 is specified, no line breaks are added
 */
void ec_fasta_write(FILE *stream, const char *id, const char *comment,
                    const char *seq, unsigned width);
#endif

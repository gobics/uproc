#ifndef UPRO_FASTA_H
#define UPRO_FASTA_H

/** \file upro/fasta.h
 *
 * Sequence input/output
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "upro/io.h"

struct upro_fasta_reader
{
    char *line;
    long line_len;
    size_t line_sz, line_no;
    char *header, *comment, *seq;
    size_t header_len, comment_len, seq_len;
    size_t header_sz, comment_sz, seq_sz, seq_sz_hint;
};

void upro_fasta_reader_init(struct upro_fasta_reader *rd, size_t seq_sz_hint);

void upro_fasta_reader_free(struct upro_fasta_reader *rd);

int upro_fasta_read(upro_io_stream *stream, struct upro_fasta_reader *rd);


/** Print FASTA formatted sequence to stream
 *
 * \param stream    stream to write to
 * \param id        sequence id
 * \param comment   comment(s) or null pointer
 * \param seq       sequence data
 * \param width     how many sequence characters are printed per line, if the
 *                  value 0 is specified, no line breaks are added
 */
void upro_fasta_write(upro_io_stream *stream, const char *id, const char *comment,
                    const char *seq, unsigned width);
#endif

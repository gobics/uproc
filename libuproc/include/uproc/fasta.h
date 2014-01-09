/** \file uproc/fasta.h
 *
 * Sequence input/output
 *
 * Copyright 2014 Peter Meinicke, Robin Martinjak
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

#ifndef UPROC_FASTA_H
#define UPROC_FASTA_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "uproc/io.h"

struct uproc_fasta_reader
{
    char *line;
    long line_len;
    size_t line_sz, line_no;
    char *header, *comment, *seq;
    size_t header_len, comment_len, seq_len;
    size_t header_sz, comment_sz, seq_sz, seq_sz_hint;
};

void uproc_fasta_reader_init(struct uproc_fasta_reader *rd, size_t seq_sz_hint);

void uproc_fasta_reader_free(struct uproc_fasta_reader *rd);

int uproc_fasta_read(uproc_io_stream *stream, struct uproc_fasta_reader *rd);

/** Print FASTA formatted sequence to stream
 *
 * \param stream    stream to write to
 * \param id        sequence id
 * \param comment   comment(s) or null pointer
 * \param seq       sequence data
 * \param width     how many sequence characters are printed per line, if the
 *                  value 0 is specified, no line breaks are added
 */
void uproc_fasta_write(uproc_io_stream *stream, const char *id,
                       const char *comment, const char *seq, unsigned width);
#endif

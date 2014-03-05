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

/** \file uproc/seqio.h
 *
 * Sequence input/output
 *
 * \weakgroup grp_io
 * \{
 *
 * \weakgroup grp_io_seqio Sequence IO
 * \{
 */

#ifndef UPROC_FASTA_H
#define UPROC_FASTA_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "uproc/io.h"


/** \defgroup struct_sequence struct uproc_sequence
 * \{
 */

/** DNA/RNA or protein sequence */
struct uproc_sequence
{
    /** Sequence identifier */
    char *header;

    /** Sequence data */
    char *data;

    /** Position in the file
     *
     * Starting position of the sequence (including header) in the file from it
     * was read. Using ::uproc_io_seek to this position <em> before creating an
     * iterator </em> will cause iteration to start with this particular
     * sequence (this is used by the python library to implement "lazy"
     * sequence objects).
     */
    long offset;
};


/** Initializer for ::uproc_sequence structs */
#define UPROC_SEQUENCE_INITIALIZER { NULL, NULL, 0 }


/** Initialize a ::uproc_sequence struct */
void uproc_sequence_init(struct uproc_sequence *seq);


/** Free allocated pointers of a ::uproc_sequence struct */
void uproc_sequence_destroy(struct uproc_sequence *seq);


/** Deep-copy a ::uproc_sequence struct */
int uproc_sequence_copy(struct uproc_sequence *dest,
                        const struct uproc_sequence *src);
/** \} */


/** Sequence file iterator
 *
 * \defgroup obj_seqiter object uproc_seqiter
 * \{
 */

typedef struct uproc_seqiter_s uproc_seqiter;

uproc_seqiter *uproc_seqiter_create(uproc_io_stream *stream,
                                    size_t seq_sz_hint);

void uproc_seqiter_destroy(uproc_seqiter *iter);

int uproc_seqiter_next(uproc_seqiter *iter, struct uproc_sequence *seq);
/** \} */

void uproc_seqio_write_fasta(uproc_io_stream *stream, const char *header,
                             const char *seq, int width);

/**
 * \}
 * \}
 */
#endif

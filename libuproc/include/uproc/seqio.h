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
 * @{
 *
 * \weakgroup grp_io_seqio Sequence IO
 * @{
 */

#ifndef UPROC_FASTA_H
#define UPROC_FASTA_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "uproc/io.h"

struct uproc_sequence
{
    char *header;
    char *seq;
    long offset;
};

typedef struct uproc_seqiter_s uproc_seqiter;

uproc_seqiter *uproc_seqiter_create(uproc_io_stream *stream,
                                    size_t seq_sz_hint);

void uproc_seqiter_destroy(uproc_seqiter *iter);

int uproc_seqiter_next(uproc_seqiter *iter, struct uproc_sequence *seq);

void uproc_seqio_write_fasta(uproc_io_stream *stream, const char *header,
                             const char *seq, int width);

/**
 * @}
 * @}
 */
#endif

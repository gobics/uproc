/* Sequence input/output
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <assert.h>

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/io.h"
#include "uproc/seqio.h"

struct uproc_seqiter_s
{
    /* associated I/O stream */
    uproc_io_stream *stream;

    /* last read line */
    char *line;

    /* length of the string in line or -1 in case of EOF or error */
    long line_len;

    /* line buffer size */
    size_t line_sz;

    /* line number */
    size_t line_no;

    /* file offset of the current record */
    long offset;

    /* header and sequence data */
    char *header, *seq;
    /* their string lengths */

    size_t header_len, seq_len;
    /* their buffer sizes */

    size_t header_sz, seq_sz, seq_sz_hint;

    enum { UNINITIALIZED, FASTA, FASTQ } format;
};


uproc_seqiter *
uproc_seqiter_create(uproc_io_stream *stream, size_t seq_sz_hint)
{
    struct uproc_seqiter_s *iter = malloc(sizeof *iter);
    if (!iter) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }

    *iter = (struct uproc_seqiter_s) {
        .stream = stream,
        .line_no = 1,
        .format = UNINITIALIZED,
    };

    if (seq_sz_hint) {
        iter->seq = malloc(seq_sz_hint);
        if (!iter->seq) {
            uproc_error(UPROC_ENOMEM);
            free(iter);
            return NULL;
        }
        iter->seq_sz = iter->seq_sz_hint = seq_sz_hint;
    }
    return iter;
}


void
uproc_seqiter_destroy(uproc_seqiter *iter)
{
    if (!iter) {
        return;
    }
    free(iter->line);
    free(iter->header);
    free(iter->seq);
    free(iter);
}


static void
iter_getline(struct uproc_seqiter_s *iter)
{
    iter->offset = uproc_io_tell(iter->stream);
    iter->line_len = uproc_io_getline(&iter->line, &iter->line_sz,
                                      iter->stream);
    if (iter->line_len >= 0) {
        iter->line_no++;
    }
}


static int
iter_realloc_header(struct uproc_seqiter_s *iter, size_t sz)
{
    if (iter->header_sz < sz) {
        void *tmp = realloc(iter->header, sz);
        if (!tmp) {
            return uproc_error(UPROC_ENOMEM);
        }
        iter->header = tmp;
        iter->header_sz = sz;
    }
    return 0;
}


static int
iter_realloc_seq(struct uproc_seqiter_s *iter, size_t sz)
{
    if (iter->seq_sz < sz) {
        void *tmp = realloc(iter->seq, sz);
        if (!tmp) {
            return uproc_error(UPROC_ENOMEM);
        }
        iter->seq = tmp;
        iter->seq_sz = sz;
    }
    return 0;
}


static int
read_fasta(struct uproc_seqiter_s *iter)
{
    size_t len, total_len;
    /* header needs at least '>', one character, '\n' */
    if (iter->line_len < 3 || iter->line[0] != '>') {
        return uproc_error_msg(UPROC_EINVAL,
                               "expected fasta header in line %zu",
                               iter->line_no);
    }

    len = iter->line_len - 1;
    if (iter_realloc_header(iter, len)) {
        return -1;

    }
    memcpy(iter->header, iter->line + 1, len - 1);
    iter->header[len - 1] = '\0';
    iter->header_len = len - 1;
    assert(iter->header_len == strlen(iter->header));

    iter_getline(iter);
    if (iter->line_len == -1) {
        if (uproc_errno == UPROC_ERRNO) {
            return -1;
        }
        return uproc_error_msg(UPROC_EINVAL,
                              "expected line after header (line %zu)",
                              iter->line_no);
    }

    /* skip ALL the comments! */
    while (iter->line[0] == ';' && iter->line_len != -1) {
        iter_getline(iter);
    }

    for (total_len = 0;
         iter->line[0] != '>' && iter->line_len != -1;
         iter_getline(iter))
    {
        len = iter->line_len;
        if (iter_realloc_seq(iter, total_len + len)) {
            return -1;
        }
        if (iter->line[len - 1] == '\n') {
            len--;
        }
        memcpy(iter->seq + total_len, iter->line, len);
        total_len += len;
        iter->seq[total_len] = '\0';
    }
    iter->seq_len = total_len;
    assert(iter->seq_len == strlen(iter->seq));

    return 0;
}


static int
read_fastq(struct uproc_seqiter_s *iter)
{
    size_t len;
    if (iter->line_len < 3 || iter->line[0] != '@') {
        return uproc_error_msg(UPROC_EINVAL,
                               "expected fastq header in line %zu",
                               iter->line_no);
    }
    len = iter->line_len - 1;
    if (iter_realloc_header(iter, len)) {
        return -1;
    }
    memcpy(iter->header, iter->line + 1, len - 1);
    iter->header[len - 1] = '\0';
    iter->header_len = len - 1;
    assert(iter->header_len == strlen(iter->header));

    iter_getline(iter);
    if (iter->line_len == -1) {
        if (uproc_errno == UPROC_ERRNO) {
            return -1;
        }
        return uproc_error_msg(UPROC_EINVAL,
                              "expected line after header (line %zu)",
                              iter->line_no);
    }
    len = iter->line_len - 1;
    if (iter_realloc_seq(iter, len)) {
        return -1;
    }
    memcpy(iter->seq, iter->line, len);
    iter->seq[len] = '\0';
    iter->seq_len = len;

    /* skip the '+' line that repeats the header */
    iter_getline(iter);
    if (iter->line_len == -1 || iter->line[0] != '+') {

        return uproc_error_msg(UPROC_EINVAL,
                               "expected line beginning with '+' (line %zu)",
                               iter->line_no);
    }

    /* skip qualities */
    iter_getline(iter);
    if (iter->line_len == -1) {
        return uproc_error_msg(UPROC_EINVAL,
                               "expected \"qualities\" (line %zu)\n",
                               iter->line_no);
    }

    /* get line for next iteration */
    iter_getline(iter);
    return 0;
}


int
uproc_seqiter_next(uproc_seqiter *iter, struct uproc_sequence *seq)
{
    /* the previously yielded sequence was the last one in the file */
    if (iter->line_len == -1) {
        return 1;
    }

    /* set beginning offset BEFORE any possible reading operation */
    seq->offset = iter->offset;

    /* fist iteration: guess file format */
    if (iter->format == UNINITIALIZED) {
        if (!iter->line) {
            iter_getline(iter);
        }
        if (iter->line_len == -1) {
            return -1;
        }

        /* guess the format  from the first character */
        if (iter->line[0] == '>') {
            iter->format = FASTA;
        }
        else if (iter->line[0] == '@') {
            iter->format = FASTQ;
        }
        else {
            return uproc_error_msg(UPROC_EINVAL,
                                   "Unknown sequence format\n");
        }
    }

    int res;
    switch (iter->format) {
        case FASTA:
            res = read_fasta(iter);
            break;

        case FASTQ:
            res = read_fastq(iter);
            break;

        default:
            return uproc_error_msg(UPROC_EINVAL, "invalid sequence iterator");
    }

    if (res) {
        return res;
    }

    seq->header = iter->header;
    seq->seq = iter->seq;
    return 0;
}


void
uproc_seqio_write_fasta(uproc_io_stream *stream, const char *header,
                        const char *seq, int width)
{
    uproc_io_printf(stream, ">%s\n", header);

    if (width) {
        const char *s = seq;
        while (*s) {
            s += uproc_io_printf(stream, "%.*s\n", width, s) - 1;
        }
    }
    else {
        uproc_io_printf(stream, "%s\n", seq);
    }
}


void
uproc_sequence_init(struct uproc_sequence *seq)
{
    *seq = (struct uproc_sequence) UPROC_SEQUENCE_INITIALIZER;
}

void
uproc_sequence_free(struct uproc_sequence *seq)
{
    free(seq->header);
    seq->header = NULL;
    free(seq->seq);
    seq->seq = NULL;
}

int
uproc_sequence_copy(struct uproc_sequence *dest, const struct uproc_sequence *src)
{
    char *h = strdup(src->header);
    char *d = strdup(src->seq);
    if (!h || !d) {
        free(h);
        free(d);
        return uproc_error(UPROC_ENOMEM);
    }
    *dest = *src;
    dest->header = h;
    dest->seq = d;
    return 0;
}

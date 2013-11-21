#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <assert.h>

#include "upro/common.h"
#include "upro/error.h"
#include "upro/io.h"
#include "upro/fasta.h"

void
upro_fasta_reader_init(struct upro_fasta_reader *rd, size_t seq_sz_hint)
{
    *rd = (struct upro_fasta_reader) {
        .seq_sz_hint = seq_sz_hint,
        .line_no = 1
    };
}

void
upro_fasta_reader_free(struct upro_fasta_reader *rd)
{
    free(rd->line);
    free(rd->header);
    free(rd->comment);
    free(rd->seq);
}

static void
reader_getline(upro_io_stream *stream, struct upro_fasta_reader *rd)
{
    rd->line_len = upro_io_getline(&rd->line, &rd->line_sz, stream);
    if (rd->line_len >= 0) {
        rd->line_no++;
    }
}

int
upro_fasta_read(upro_io_stream *stream, struct upro_fasta_reader *rd)
{
    size_t len, total_len;
    if (!rd->header) {
        rd->header_sz = 0;
    }
    if (!rd->comment) {
        rd->comment_sz = 0;
    }
    if (!rd->seq) {
        if (rd->seq_sz_hint) {
            rd->seq = malloc(rd->seq_sz_hint);
            if (!rd->seq) {
                return upro_error(UPRO_ENOMEM);
            }
        }
        rd->seq_sz = rd->seq_sz_hint;
    }

    if (!rd->line) {
        reader_getline(stream, rd);
    }

    if (rd->line_len == -1) {
        return UPRO_SUCCESS;
    }

    /* header needs at least '>', one character, '\n' */
    if (rd->line_len < 3 || rd->line[0] != '>') {
        return upro_error_msg(UPRO_EINVAL,
                              "invalid fasta header in line %zu", rd->line_no);
    }

    len = rd->line_len - 1;
    if (rd->header_sz < len) {
        void *tmp = realloc(rd->header, len);
        if (!tmp) {
            return upro_error(UPRO_ENOMEM);
        }
        rd->header = tmp;
        rd->header_sz = len;
    }
    memcpy(rd->header, rd->line + 1, len - 1);
    rd->header[len - 1] = '\0';
    rd->header_len = len - 1;
    assert(rd->header_len == strlen(rd->header));

    reader_getline(stream, rd);
    if (rd->line_len == -1) {
        if (upro_errno == UPRO_ESYSCALL) {
            return UPRO_FAILURE;
        }
        return upro_error_msg(UPRO_EINVAL,
                              "expected line after header (line %zu)",
                              rd->line_no);
    }

    for (total_len = 0;
         rd->line[0] == ';' && rd->line_len != -1;
         reader_getline(stream, rd))
    {
        len = rd->line_len;
        if (rd->comment_sz < total_len + len) {
            void *tmp = realloc(rd->comment, total_len + len);
            if (!tmp) {
                return upro_error(UPRO_ENOMEM);
            }
            rd->comment = tmp;
            rd->comment_sz = total_len + len;
        }
        /* skip leading ';' but keep newline */
        memcpy(rd->comment + total_len, rd->line + 1, len - 1);
        total_len += len - 1;
    }
    if (total_len > 1) {
        /* remove last newline */
        rd->comment[total_len - 1] = '\0';
    }
    rd->comment_len = total_len - 1;
    assert(!rd->comment || rd->comment_len == strlen(rd->comment));

    for (total_len = 0;
         rd->line[0] != '>' && rd->line_len != -1;
         reader_getline(stream, rd))
    {
        len = rd->line_len;
        if (rd->seq_sz < total_len + len) {
            void *tmp = realloc(rd->seq, total_len + len);
            if (!tmp) {
                return upro_error(UPRO_ENOMEM);
            }
            rd->seq = tmp;
            rd->seq_sz = total_len + len;
        }
        if (rd->line[len - 1] == '\n') {
            len--;
        }
        memcpy(rd->seq + total_len, rd->line, len);
        total_len += len;
        rd->seq[total_len] = '\0';
    }
    rd->seq_len = total_len;
    assert(rd->seq_len == strlen(rd->seq));

    return UPRO_ITER_YIELD;
}

void
upro_fasta_write(upro_io_stream *stream, const char *id, const char *comment,
                 const char *seq, unsigned width)
{
    upro_io_printf(stream, ">%s\n", id);

    if (comment && *comment) {
        const char *c = comment, *nl;
        while (*c) {
            nl = strchr(c, '\n');
            if (!nl) {
                nl = c + strlen(c);
            }
            upro_io_printf(stream, ";%.*s\n", (int)(nl - c), c);
            c = nl + !!*nl;
        }
    }

    if (!width) {
        upro_io_printf(stream, "%s\n", seq);
        return;
    }

    while (*seq) {
        seq += upro_io_printf(stream, "%.*s\n", width, seq) - 1;
    }
}

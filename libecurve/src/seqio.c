#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>

#include "ecurve/common.h"
#include "ecurve/seqio.h"

#define BUFSZ_INIT 500
#define BUFSZ_STEP 100

#define MARKER_ID '>'
#define MARKER_COMMENT ';'

static void
skip_comments(FILE *stream)
{
    int c, d;
    while ((c = fgetc(stream)) == MARKER_COMMENT) {
        while ((d = fgetc(stream)) != EOF && d != '\n') {
            ;
        }
    }
    ungetc(c, stream);
}

static int
grow_buf(char **buf, size_t *n)
{
    char *tmp = realloc(*buf, *n + BUFSZ_STEP);
    if (!tmp) {
        return EC_ENOMEM;
    }
    *n += BUFSZ_STEP;
    *buf = tmp;
    return EC_SUCCESS;
}

void
ec_seqio_filter_init(struct ec_seqio_filter *filter,
                     enum ec_seqio_filter_flags flags,
                     enum ec_seqio_filter_mode default_mode,
                     const char *accept, const char *ignore, const char *warn)
{
    int i;
    const char *p;
    for (i = 0; i < (1 << CHAR_BIT); i++) {
        filter->mode[(unsigned char) i] = default_mode;
    }
    for (p = accept; p && *p; p++) {
        filter->mode[(unsigned char) *p] = EC_SEQIO_ACCEPT;
    }
    for (p = ignore; p && *p; p++) {
        filter->mode[(unsigned char) *p] = EC_SEQIO_IGNORE;
    }
    for (p = warn; p && *p; p++) {
        filter->mode[(unsigned char) *p] = EC_SEQIO_WARN;
    }
    filter->flags = flags;
    filter->ignored = filter->warnings = 0;
}

int
ec_seqio_fasta_read(FILE *stream, struct ec_seqio_filter *filter,
                    char **id, size_t *id_size,
                    char **comment, size_t *comment_size,
                    char **seq, size_t *seq_size)
{
    int c, d;
    size_t i;

#define INIT_BUF(buf, sz) do {                              \
    if (!*buf) {                                            \
        if (!(*buf = malloc(BUFSZ_INIT))) {                 \
            return EC_ENOMEM;                               \
        }                                                   \
        *sz = BUFSZ_INIT;                                   \
    }                                                       \
} while (0)

#define GROW_BUF(buf, sz) do {                              \
    if (i >= *sz - 1) {                                     \
        int res = grow_buf(buf, sz);                        \
        if (EC_ISERROR(res)) {                              \
            return res;                                     \
        }                                                   \
    }                                                       \
} while (0)

    /* reset `filter` counts */
    if (filter && (filter->flags & EC_SEQIO_F_RESET)) {
        filter->ignored = filter->warnings = 0;
    }

    /* initialize buffers if needed */
    INIT_BUF(id, id_size);
    INIT_BUF(seq, seq_size);
    if (comment) {
        INIT_BUF(comment, comment_size);
    }

    /* fasta ID line starts with '>', everything else is an error */
    if ((c = fgetc(stream)) != MARKER_ID) {
        if (c == EOF) {
            return EC_ITER_STOP;
        }
        ungetc(c, stream);
        return EC_EINVAL;
    }

    /* read ID */
    i = 0;
    while ((c = fgetc(stream)) != EOF && c != '\n') {
        GROW_BUF(id, id_size);
        (*id)[i++] = c;
    }
    (*id)[i] = '\0';

    /* read all comments */
    if (comment) {
        i = 0;
        **comment = '\0';
        while ((c = fgetc(stream)) == MARKER_COMMENT) {
            if (i) {
                GROW_BUF(comment, comment_size);
                (*comment)[i++] = '\n';
            }
            while ((d = fgetc(stream)) != EOF && d != '\n') {
                GROW_BUF(comment, comment_size);
                (*comment)[i++] = d;
            }
        }
        (*comment)[i] = '\0';
        ungetc(c, stream);
    }
    /* or skip them */
    else {
        skip_comments(stream);
    }

    /* read sequence */
    i = 0;
    while ((c = toupper(fgetc(stream))) != EOF) {
        if (c == '\n') {
            d = ungetc(fgetc(stream), stream);
            if (d == EOF || d == MARKER_ID)
                break;
        }

        /* ignore whitespace */
        if (isspace(c)) {
            continue;
        }

        /* apply filter */
        if (filter) {
            enum ec_seqio_filter_mode f = filter->mode[(unsigned char) c];
            if (f == EC_SEQIO_IGNORE) {
                filter->ignored++;
                continue;
            }
            else if (f == EC_SEQIO_WARN) {
                filter->warnings++;
            }
        }

        GROW_BUF(seq, seq_size);
        (*seq)[i++] = c;
    }
    (*seq)[i] = '\0';
    return EC_ITER_YIELD;
}

void
ec_seqio_fasta_print(FILE *stream, const char *id, const char *comment,
                     const char *seq, unsigned width)
{
    fprintf(stream, ">%s\n", id);

    if (comment && *comment) {
        char c;
        fprintf(stream, ";");

        while ((c = *comment++)) {
            putc(c, stream);
            if (c == '\n') {
                fprintf(stream, ";");
            }
        }
        putc('\n', stream);
    }

    if (!width) {
        fprintf(stream, "%s\n", seq);
        return;
    }

    while (*seq) {
        seq += fprintf(stream, "%.*s", width, seq);
        fputc('\n', stream);
    }
}

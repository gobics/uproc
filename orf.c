#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "uproc.h"


enum opts {
    CODON_SCORES = 1,
    MIN_SCORE,
    INFILE,
    ARGC,
};

bool
orf_filter(const struct uproc_orf *orf, const char *seq, size_t seq_len,
        double seq_gc, void *opaque)
{
    size_t r, c, rows, cols;
    struct uproc_matrix *thresh = opaque;
    (void) seq;
    if (orf->length < 20) {
        return false;
    }
    if (!thresh) {
        return true;
    }
    uproc_matrix_dimensions(thresh, &rows, &cols);
    r = seq_gc * 100;
    c = seq_len;
    if (r >= rows) {
        r = rows - 1;
    }
    if (c >= cols) {
        c = cols - 1;
    }
    return orf->score >= uproc_matrix_get(thresh, r, c);
}

int main(int argc, char **argv)
{
    int res;
    struct uproc_matrix codon_scores_mat;
    double codon_scores[UPROC_BINARY_CODON_COUNT];

    struct uproc_matrix orf_thresholds;
    char *endptr;
    double min_score;

    struct uproc_fasta_reader rd;
    uproc_io_stream *stream;

    enum { MAX, ALL } mode;
    void *filter_arg = NULL;

    if (argc < ARGC) {
        fprintf(stderr,
                "usage: %s codon_scores_file (\"max\"|number|thresholds_file) [input_file]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    res = uproc_matrix_load(&codon_scores_mat, UPROC_IO_GZIP, argv[CODON_SCORES]);
    if (res) {
        uproc_perror("error loading \"%s\"", argv[CODON_SCORES]);
        return EXIT_FAILURE;
    }
    uproc_orf_codonscores(codon_scores, &codon_scores_mat);

    if (!strcasecmp(argv[MIN_SCORE], "MAX")) {
        mode = MAX;
    }
    else {
        mode = ALL;
        min_score = strtod(argv[MIN_SCORE], &endptr);
        if (*argv[MIN_SCORE] && *endptr) {
            res = uproc_matrix_load(&orf_thresholds, UPROC_IO_GZIP, argv[MIN_SCORE]);
            if (res) {
                uproc_perror("error loading \"%s\"", argv[MIN_SCORE]);
                return EXIT_FAILURE;
            }
        }
        else {
            uproc_matrix_init(&orf_thresholds, 1, 1, &min_score);
        }

        filter_arg = &orf_thresholds;
    }

    if (argc == ARGC && strcmp(argv[INFILE], "-")) {
        stream = uproc_io_open("r", UPROC_IO_GZIP, argv[INFILE]);
        if (!stream) {
            perror("");
            return EXIT_FAILURE;
        }
    }
    else {
        stream = uproc_stdin;
    }

    uproc_fasta_reader_init(&rd, 4096);
    while ((res = uproc_fasta_read(stream, &rd)) > 0) {
        struct uproc_orfiter oi;
        struct uproc_orf orf;

        char *max_orf = NULL;
        double max_score = -INFINITY;
        uproc_orfiter_init(&oi, rd.seq, codon_scores, orf_filter, filter_arg);
        while (uproc_orfiter_next(&oi, &orf) > 0) {
            if (mode == MAX && orf.score > max_score) {
                free(max_orf);
                max_orf = strdup(orf.data);
            }
            else {
                uproc_fasta_write(uproc_stdout, rd.header, rd.comment, orf.data, 0);
            }
        }
        if (mode == MAX && max_orf) {
            uproc_fasta_write(uproc_stdout, rd.header, rd.comment, max_orf, 0);
            free(max_orf);
        }
        uproc_orfiter_destroy(&oi);
    }
    if (res < 0) {
        uproc_perror("error reading input");
    }
    return EXIT_SUCCESS;
}

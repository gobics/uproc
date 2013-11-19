#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "upro.h"


enum opts {
    CODON_SCORES = 1,
    MIN_SCORE,
    INFILE,
    ARGC,
};

bool
orf_filter(const struct upro_orf *orf, const char *seq, size_t seq_len,
        double seq_gc, void *opaque)
{
    size_t r, c, rows, cols;
    struct upro_matrix *thresh = opaque;
    (void) seq;
    if (orf->length < 20) {
        return false;
    }
    if (!thresh) {
        return true;
    }
    upro_matrix_dimensions(thresh, &rows, &cols);
    r = seq_gc * 100;
    c = seq_len;
    if (r >= rows) {
        r = rows - 1;
    }
    if (c >= cols) {
        c = cols - 1;
    }
    return orf->score >= upro_matrix_get(thresh, r, c);
}

int main(int argc, char **argv)
{
    int res;
    struct upro_matrix codon_scores_mat;
    double codon_scores[UPRO_BINARY_CODON_COUNT];

    struct upro_matrix orf_thresholds;
    char *endptr;
    double min_score;

    struct upro_fasta_reader rd;
    upro_io_stream *stream;

    enum { MAX, ALL } mode;
    void *filter_arg = NULL;

    if (argc < ARGC) {
        fprintf(stderr,
                "usage: %s codon_scores_file (\"max\"|number|thresholds_file) [input_file]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    res = upro_matrix_load(&codon_scores_mat, UPRO_IO_GZIP, argv[CODON_SCORES]);
    if (UPRO_ISERROR(res)) {
        fprintf(stderr, "can't open file \"%s\"\n", argv[CODON_SCORES]);
        if (res == UPRO_ESYSCALL) {
            perror("");
        }
        return EXIT_FAILURE;
    }
    upro_orf_codonscores(codon_scores, &codon_scores_mat);

    if (!strcasecmp(argv[MIN_SCORE], "MAX")) {
        mode = MAX;
    }
    else {
        mode = ALL;
        min_score = strtod(argv[MIN_SCORE], &endptr);
        if (*argv[MIN_SCORE] && *endptr) {
            res = upro_matrix_load(&orf_thresholds, UPRO_IO_GZIP, argv[MIN_SCORE]);
            if (UPRO_ISERROR(res)) {
                fprintf(stderr, "can't open file \"%s\"\n", argv[MIN_SCORE]);
                if (res == UPRO_ESYSCALL) {
                    perror("");
                }
                return EXIT_FAILURE;
            }
        }
        else {
            upro_matrix_init(&orf_thresholds, 1, 1, &min_score);
        }

        filter_arg = &orf_thresholds;
    }

    if (argc == ARGC && strcmp(argv[INFILE], "-")) {
        stream = upro_io_open("r", UPRO_IO_GZIP, argv[INFILE]);
        if (!stream) {
            perror("");
            return EXIT_FAILURE;
        }
    }
    else {
        stream = upro_stdin;
    }

    upro_fasta_reader_init(&rd, 4096);
    while ((res = upro_fasta_read(stream, &rd)) == UPRO_ITER_YIELD) {
        struct upro_orfiter oi;
        struct upro_orf orf;

        char *max_orf = NULL;
        double max_score = -INFINITY;
        upro_orfiter_init(&oi, rd.seq, codon_scores, orf_filter, filter_arg);
        while (upro_orfiter_next(&oi, &orf) == UPRO_ITER_YIELD) {
            if (mode == MAX && orf.score > max_score) {
                free(max_orf);
                max_orf = strdup(orf.data);
            }
            else {
                upro_fasta_write(upro_stdout, rd.header, rd.comment, orf.data, 0);
            }
        }
        if (mode == MAX && max_orf) {
            upro_fasta_write(upro_stdout, rd.header, rd.comment, max_orf, 0);
            free(max_orf);
        }
        upro_orfiter_destroy(&oi);
    }
    if (res != UPRO_ITER_STOP) {
        fprintf(stderr, "an error occured\n");
    }
    return EXIT_SUCCESS;
}

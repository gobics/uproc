#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "ecurve.h"


enum opts {
    CODON_SCORES = 1,
    MIN_SCORE,
    INFILE,
    ARGC,
};

bool
orf_filter(const struct ec_orf *orf, const char *seq, size_t seq_len,
        double seq_gc, void *opaque)
{
    size_t r, c, rows, cols;
    struct ec_matrix *thresh = opaque;
    (void) seq;
    if (orf->length < 60) {
        return false;
    }
    if (!thresh) {
        return true;
    }
    ec_matrix_dimensions(thresh, &rows, &cols);
    r = seq_gc * 100;
    c = seq_len;
    if (r >= rows) {
        r = rows - 1;
    }
    if (c >= cols) {
        c = cols - 1;
    }
    return orf->score >= ec_matrix_get(thresh, r, c);
}

int main(int argc, char **argv)
{
    int res;
    struct ec_orf_codonscores codon_scores;

    struct ec_matrix orf_thresholds;
    char *endptr;
    double min_score;

    struct ec_fasta_reader rd;
    ec_io_stream *stream;

    enum { MAX, ALL } mode;
    void *filter_arg = NULL;

    if (argc < ARGC) {
        fprintf(stderr,
                "usage: %s codon_scores_file (\"max\"|number|thresholds_file) [input_file]\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    res = ec_orf_codonscores_load_file(&codon_scores, argv[CODON_SCORES], EC_IO_GZIP);
    if (EC_ISERROR(res)) {
        fprintf(stderr, "can't open file \"%s\"\n", argv[CODON_SCORES]);
        if (res == EC_ESYSCALL) {
            perror("");
        }
        return EXIT_FAILURE;
    }

    if (!strcasecmp(argv[MIN_SCORE], "MAX")) {
        mode = MAX;
    }
    else {
        mode = ALL;
        min_score = strtod(argv[MIN_SCORE], &endptr);
        if (*argv[MIN_SCORE] && *endptr) {
            res = ec_matrix_load_file(&orf_thresholds, argv[MIN_SCORE], EC_IO_GZIP);
            if (EC_ISERROR(res)) {
                fprintf(stderr, "can't open file \"%s\"\n", argv[MIN_SCORE]);
                if (res == EC_ESYSCALL) {
                    perror("");
                }
                return EXIT_FAILURE;
            }
        }
        else {
            ec_matrix_init(&orf_thresholds, 1, 1, &min_score);
        }

        filter_arg = &orf_thresholds;
    }

    if (argc == ARGC && strcmp(argv[INFILE], "-")) {
        stream = ec_io_open(argv[INFILE], "r", EC_IO_GZIP);
        if (!stream) {
            perror("");
            return EXIT_FAILURE;
        }
    }
    else {
        stream = ec_stdin;
    }

    ec_fasta_reader_init(&rd, 4096);
    while ((res = ec_fasta_read(stream, &rd)) == EC_ITER_YIELD) {
        struct ec_orfiter oi;
        struct ec_orf orf;

        char *max_orf = NULL;
        double max_score = -INFINITY;
        ec_orfiter_init(&oi, rd.seq, &codon_scores, orf_filter, filter_arg);
        while (ec_orfiter_next(&oi, &orf)) {
            if (mode == MAX && orf.score > max_score) {
                free(max_orf);
                max_orf = strdup(orf.data);
            }
            else {
                ec_fasta_write(ec_stdout, rd.header, rd.comment, orf.data, 0);
            }
        }
        if (mode == MAX && max_orf) {
            ec_fasta_write(ec_stdout, rd.header, rd.comment, max_orf, 0);
            free(max_orf);
        }
        ec_orfiter_destroy(&oi);
    }
    if (res != EC_ITER_STOP) {
        fprintf(stderr, "an error occured\n");
    }
    return EXIT_SUCCESS;
}

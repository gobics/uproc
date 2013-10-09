#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ecurve.h"


enum opts {
    CODON_SCORES = 1,
    MIN_SCORE,
    INFILE,
    ARGC,
};

int main(int argc, char **argv)
{
    int res;
    struct ec_orf_codonscores codon_scores;

    struct ec_matrix orf_thresholds;
    char *endptr;
    double min_score;

    struct ec_fasta_reader rd;
    ec_io_stream *stream;

    char *buf = NULL;
    size_t sz = 0;

    enum { MAX, SCORE } mode;

    if (argc < ARGC) {
        fprintf(stderr,
                "usage: %s codon_scores_file (number|thresholds_file) [input_file]\n",
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
        mode = SCORE;
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
        if (mode == MAX) {
            res = ec_orf_max(rd.seq, &codon_scores, &buf, &sz);
        }
        else {
            res = ec_orf_chained(rd.seq, EC_ORF_ALL, &codon_scores, &orf_thresholds, &buf, &sz);
        }
        if (EC_ISERROR(res)) {
            break;
        }
        if (buf && *buf) {
            ec_fasta_write(ec_stdout, rd.header, rd.comment, buf, 0);
        }
    }
    if (res != EC_ITER_STOP) {
        fprintf(stderr, "an error occured\n");
    }
    free(buf);
}

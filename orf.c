/* uproc-orf
 * Extract open reading frames from DNA/RNA sequences.
 *
 * Copyright 2014 Peter Meinicke, Robin Martinjak
 *
 * This file is part of uproc.
 *
 * uproc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * uproc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with uproc.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <uproc.h>

#define PROGNAME "uproc-orf"

struct orf_filter_arg
{
    unsigned min_length;
    uproc_matrix *thresh;
};

bool
orf_filter(const struct uproc_orf *orf, const char *seq, size_t seq_len,
           double seq_gc, void *opaque)
{
    size_t r, c, rows, cols;
    struct orf_filter_arg *arg = opaque;
    (void) seq;
    if (orf->length < arg->min_length) {
        return false;
    }
    if (!arg->thresh) {
        return true;
    }
    uproc_matrix_dimensions(arg->thresh, &rows, &cols);
    r = seq_gc * 100;
    c = seq_len;
    if (r >= rows) {
        r = rows - 1;
    }
    if (c >= cols) {
        c = cols - 1;
    }
    return orf->score >= uproc_matrix_get(arg->thresh, r, c);
}

void
print_usage(const char *progname)
{
    fprintf(
        stderr,
        PROGNAME ", version " PACKAGE_VERSION "\n"
        "\n"
        "USAGE: %s [options] [INPUTFILES] \n"
        "Translate DNA/RNA to protein sequences\n"
        "INPUTFILES can be zero or more files containing sequences in FASTA\n"
        "or FASTQ format (FASTQ qualities are ignored).\n"
        "If no file is specified or the file name is -, sequences\n"
        "will be read from standard input.\n"
        "\n"
        "GENERAL OPTIONS:\n"
        OPT("-h", "--help      ", "   ") "Print this message and exit.\n"
        OPT("-v", "--version   ", "   ") "Print version and exit.\n"
        OPT("-L", "--min-length", "N  ") "Minimum ORF length (Default: 20)\n"
        OPT("-m", "--model     ", "DIR") "Score ORFs using the model in DIR.\n"
        "\n"
        "If -m is omitted, all ORFs with length greater or equal to the\n"
        "minimum length are output. If -m is used, ORFs are scored using the\n"
        "according codon scores and can be filtered using the options below.\n"
        "By default \"-O 2\" is used.\n"
        "\n"
        "SCORING OPTIONS:\n"
        OPT("-O", "--othresh   ", "N  ") "ORF translation threshold level.\n"
        OPT("-S", "--min-score ", "VAL")
            "Use fixed threshold of VAL (decimal number).\n"
        OPT("-M", "--max       ", "   ")
            "Only output the ORF with the maximum score.\n"
        ,
        progname);

}

enum args
{
    INFILES,
    ARGC,
};

int main(int argc, char **argv)
{
    int res = 0, opt;
    struct orf_filter_arg filter_arg = { 20, NULL };
    const char *model_dir = NULL;

    double codon_scores[UPROC_BINARY_CODON_COUNT];
    uproc_matrix *orf_thresh = NULL;

    enum { NONE, MODEL, VALUE, MAX } thresh_mode = NONE;
    int orf_thresh_num = 2;

#define SHORT_OPTS "hvL:m:O:S:M"
#if HAVE_GETOPT_LONG
    struct option long_opts[] = {
        { "help",       no_argument,        NULL, 'h' },
        { "version",    no_argument,        NULL, 'v' },
        { "min-length", required_argument,  NULL, 'L' },
        { "model",      required_argument,  NULL, 'm' },
        { "othresh",    required_argument,  NULL, 'O' },
        { "min-score",  required_argument,  NULL, 'S' },
        { "max",        no_argument,        NULL, 'M' },
        { 0, 0, 0, 0 }
    };
#else
#define long_opts NULL
#endif
    while ((opt = getopt_long(argc, argv, SHORT_OPTS, long_opts, NULL)) != -1)
    {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            case 'v':
                print_version(PROGNAME);
                return EXIT_SUCCESS;
            case 'L':
                {
                    int res, tmp;
                    res = parse_int(optarg, &tmp);
                    if (res || tmp <= 0) {
                        fprintf(stderr, "-L requires a positive integer\n");
                        return EXIT_FAILURE;
                    }
                    filter_arg.min_length = tmp;
                }
                break;
            case 'm':
                if (thresh_mode == NONE) {
                    thresh_mode = MODEL;
                }
                model_dir = optarg;
                break;
            case 'O':
                {
                    int tmp = 42;
                    (void) parse_int(optarg, &tmp);
                    if (tmp != 0 && tmp != 1 && tmp != 2) {
                        fprintf(stderr, "-O argument must be 0, 1 or 2\n");
                        return EXIT_FAILURE;
                    }
                    orf_thresh_num = tmp;
                    thresh_mode = MODEL;
                }
                break;
            case 'S':
                {
                    double min_score;
                    char *endptr;
                    min_score = strtod(optarg, &endptr);
                    if (!*optarg || *endptr) {
                        fprintf(stderr,
                                "-S argument must be a decimal number\n");
                        return EXIT_FAILURE;
                    }
                    orf_thresh = uproc_matrix_create(1, 1, &min_score);
                    if (!orf_thresh) {
                        uproc_perror("");
                        return EXIT_FAILURE;
                    }
                    filter_arg.thresh = orf_thresh;
                    thresh_mode = VALUE;
                }
                break;
            case 'M':
                {
                    thresh_mode = MAX;
                }
                break;

            case '?':
                return EXIT_FAILURE;
        }
    }

    struct model model = MODEL_INITIALIZER;
    if (model_dir) {
        if (thresh_mode != MODEL) {
            orf_thresh_num = 0;
        }
        res = model_load(&model, model_dir, orf_thresh_num);
        if (res) {
            uproc_perror("error reading model");
            return EXIT_FAILURE;
        }
        uproc_orf_codonscores(codon_scores, model.codon_scores);

        filter_arg.thresh = model.orf_thresh;
    }
    else if (!model_dir && thresh_mode != NONE) {
        fprintf(stderr, "Error: -O, S or -M used without -m.\n");
        return EXIT_FAILURE;
    }

    if (argc < optind + ARGC - 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (argc < optind + ARGC) {
        argv[argc++] = "-";
    }

    for (; optind + INFILES < argc; optind++) {
        struct uproc_io_stream *stream;
        uproc_seqiter *rd;
        struct uproc_sequence seq;
        if (!strcmp(argv[optind + INFILES], "-")) {
            stream = uproc_stdin;
        }
        else {
            stream = uproc_io_open("r", UPROC_IO_GZIP, argv[optind + INFILES]);
            if (!stream) {
                fprintf(stderr, "error opening %s: ", argv[optind + INFILES]);
                perror("");
                return EXIT_FAILURE;
            }
        }
        rd = uproc_seqiter_create(stream);
        if (!rd) {
            uproc_perror("");
            return EXIT_FAILURE;
        }

        while (res = uproc_seqiter_next(rd, &seq), !res) {
            uproc_orfiter *oi;
            struct uproc_orf orf;

            char *max_orf = NULL;
            double max_score = -INFINITY;
            oi = uproc_orfiter_create(seq.data,
                                      thresh_mode == NONE ? NULL : codon_scores,
                                      orf_filter,
                                      &filter_arg);
            while (res = uproc_orfiter_next(oi, &orf), !res) {
                if (thresh_mode == MAX && orf.score > max_score) {
                    free(max_orf);
                    max_orf = strdup(orf.data);
                    if (!max_orf) {
                        res = uproc_error(UPROC_ENOMEM);
                        break;
                    }
                }
                else {
                    uproc_seqio_write_fasta(uproc_stdout, seq.header, orf.data,
                                            0);
                }
            }
            if (thresh_mode == MAX && max_orf) {
                if (res == 1) {
                    uproc_seqio_write_fasta(uproc_stdout, seq.header, max_orf,
                                            0);
                }
                free(max_orf);
            }
            uproc_orfiter_destroy(oi);
            if (res == -1) {
                break;
            }
        }
        uproc_seqiter_destroy(rd);
        uproc_io_close(stream);
    }
    if (res == -1) {
        uproc_perror("error reading input");
    }
    model_free(&model);
    return EXIT_SUCCESS;
}

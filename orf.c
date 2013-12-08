#if HAVE_CONFIG_H
#include <config.h>
#endif
#define PROGNAME "uproc-orf"
#include "uproc_opt.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "uproc.h"

struct orf_filter_arg
{
    unsigned min_length;
    struct uproc_matrix *thresh;
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
        "format. If no file is specified or the file name is -, sequences\n"
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
    int res, opt;
    struct orf_filter_arg filter_arg = { 20, NULL };
    const char *model_dir = NULL;

    double codon_scores[UPROC_BINARY_CODON_COUNT];
    struct uproc_matrix orf_thresholds;

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
                print_version();
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
                    uproc_matrix_init(&orf_thresholds, 1, 1, &min_score);
                    filter_arg.thresh = &orf_thresholds;
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

    if (model_dir) {
        struct uproc_matrix cs;
        res = uproc_matrix_load(&cs, UPROC_IO_GZIP,
                                "%s/codon_scores", model_dir);
        if (res) {
            uproc_perror("can't load model from \"%s\"", model_dir);
            return EXIT_FAILURE;
        }
        uproc_orf_codonscores(codon_scores, &cs);

        if (thresh_mode == MODEL && orf_thresh_num) {
            res = uproc_matrix_load(&orf_thresholds, UPROC_IO_GZIP,
                                    "%s/orf_thresh_e%d", model_dir,
                                    orf_thresh_num);
            if (res) {
                uproc_perror("can't load ORF thresholds");
                return EXIT_FAILURE;
            }
            filter_arg.thresh = &orf_thresholds;
        }
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

    struct uproc_fasta_reader rd;
    uproc_fasta_reader_init(&rd, 4096);
    for (; optind + INFILES < argc; optind++) {
        struct uproc_io_stream *stream;
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

        while ((res = uproc_fasta_read(stream, &rd)) > 0) {
            struct uproc_orfiter oi;
            struct uproc_orf orf;

            char *max_orf = NULL;
            double max_score = -INFINITY;
            uproc_orfiter_init(&oi, rd.seq,
                               thresh_mode == NONE ? NULL : codon_scores,
                               orf_filter, &filter_arg);
            while (uproc_orfiter_next(&oi, &orf) > 0) {
                if (thresh_mode == MAX && orf.score > max_score) {
                    free(max_orf);
                    max_orf = strdup(orf.data);
                }
                else {
                    uproc_fasta_write(uproc_stdout, rd.header, rd.comment,
                                      orf.data, 0);
                }
            }
            if (thresh_mode == MAX && max_orf) {
                uproc_fasta_write(uproc_stdout, rd.header, rd.comment, max_orf,
                                  0);
                free(max_orf);
            }
            uproc_orfiter_destroy(&oi);
        }
        uproc_io_close(stream);
    }
    if (res) {
        uproc_perror("error reading input");
    }
    return EXIT_SUCCESS;
}

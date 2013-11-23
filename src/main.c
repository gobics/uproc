#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include <unistd.h>
#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

#if HAVE_OMP_H
#include <omp.h>
#endif

#include "uproc.h"


#define MAX_CHUNK_SIZE (1 << 10)

struct buffer
{
    char *header[MAX_CHUNK_SIZE];
    size_t header_sz[MAX_CHUNK_SIZE];
    char *seq[MAX_CHUNK_SIZE];
    size_t seq_sz[MAX_CHUNK_SIZE];

#if MAIN_DNA
    struct uproc_dc_results results[MAX_CHUNK_SIZE];
#else
    struct uproc_pc_results results[MAX_CHUNK_SIZE];
#endif

    size_t n;
} buf[2];

void
buf_free(struct buffer *buf) {
    for (size_t i = 0; i < MAX_CHUNK_SIZE; i++) {
        free(buf->header[i]);
        free(buf->seq[i]);
        free(buf->results[i].preds);
    }
}

int
dup_str(char **dest, size_t *dest_sz, const char *src, size_t len)
{
    len += 1;
    if (len > *dest_sz) {
        char *tmp = realloc(*dest, len);
        if (!tmp) {
            return UPROC_FAILURE;
        }
        *dest = tmp;
        *dest_sz = len;
    }
    memcpy(*dest, src, len);
    return UPROC_SUCCESS;
}

int
input_read(uproc_io_stream *stream, struct uproc_fasta_reader *rd, struct buffer *buf,
           size_t chunk_size)
{
    int res = UPROC_SUCCESS;
    size_t i;

    for (i = 0; i < chunk_size; i++) {
        char *p1, *p2;
        size_t header_len;

        res = uproc_fasta_read(stream, rd);
        if (res <= 0) {
            break;
        }

        p1 = rd->header;
        while (isspace(*p1)) {
            p1++;
        }
        p2 = strpbrk(p1, ", \f\n\r\t\v");
        if (p2) {
            header_len = p2 - p1 + 1;
            *p2 = '\0';
        }
        else {
            header_len = strlen(p1);
        }

        res = dup_str(&buf->header[i], &buf->header_sz[i], p1, header_len);
        if (res) {
            break;
        }

        res = dup_str(&buf->seq[i], &buf->seq_sz[i], rd->seq, rd->seq_len);
        if (res) {
            break;
        }
    }
    buf->n = i;

    for (; i < chunk_size; i++) {
        free(buf->seq[i]);
        buf->seq[i] = NULL;
    }
    return res;
}

bool
prot_filter(const char *seq, size_t len, uproc_family family, double score,
        void *opaque)
{
    static size_t rows, cols;
    struct uproc_matrix *thresh = opaque;
    (void) seq, (void) family;
    if (!thresh) {
        return score > 0.0;
    }
    if (!rows) {
        uproc_matrix_dimensions(thresh, &rows, &cols);
    }
    if (len >= rows) {
        len = rows - 1;
    }
    return score >= uproc_matrix_get(thresh, len, 0);
}

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

void
output(struct buffer *buf,
        uproc_io_stream *pr_stream, size_t pr_seq_offset,
        uintmax_t *counts,
        size_t *unexplained,
        size_t *total)
{
    size_t i, j;
    *total += buf->n;
    for (i = 0; i < buf->n; i++) {
        if (!buf->results[i].n) {
            if (unexplained) {
                *unexplained += 1;
            }
            continue;
        }
        for (j = 0; j < buf->results[i].n; j++) {
#if MAIN_DNA
            struct uproc_dc_pred *pred = &buf->results[i].preds[j];
            if (pr_stream) {
                uproc_io_printf(pr_stream, "%zu,%s,%u,%" UPROC_FAMILY_PRI ",%1.3f\n",
                        i + pr_seq_offset + 1,
                        buf->header[i],
                        pred->frame + 1,
                        pred->family,
                        pred->score);
            }
#else
            struct uproc_pc_pred *pred = &buf->results[i].preds[j];
            if (pr_stream) {
                uproc_io_printf(pr_stream, "%zu,%s,%" UPROC_FAMILY_PRI ",%1.3f\n",
                        i + pr_seq_offset + 1,
                        buf->header[i],
                        pred->family,
                        pred->score);
            }
#endif
            if (counts) {
                counts[pred->family] += 1;
            }
        }
    }
}

size_t
get_chunk_size(void)
{
    size_t sz;
    char *end, *value = getenv("UPROC_CHUNK_SIZE");
    if (value) {
        sz = strtoull(value, &end, 10);
        if (!*end && sz && sz < MAX_CHUNK_SIZE) {
            return sz;
        }
    }
    return MAX_CHUNK_SIZE;
}


void
print_usage(const char *progname)
{
    const char *s = "\
USAGE: %s db_dir model_dir [options] [input_file(s)]\n\
\n\
GENERAL OPTIONS:\n\
-h | --help             Print this message and exit.\n\
-v | --version          Print version and exit.\n"
#if HAVE_OMP_H
"\
-t | --threads <n>      Maximum number of threads to use. This overrides the\n\
                        environment variable OMP_NUM_THREADS. If neither is\n\
                        set, the number of available CPU cores is used."
#endif
"\n\n\
OUTPUT OPTIONS:\n\
-p | --preds    Print all classifications as CSV with the fields:\n\
                    sequence number (starting with 1)\n\
                    FASTA header up to the first whitespace\n"
#if MAIN_DNA
"\
                    frame number (1-6)\n"
#endif
"\
                    protein family\n\
                    classificaton score\n\
\n\
-c | --counts   Print number of classifications for each protein family (if\n\
                this number is greater than 0) in the format:\n\
                    \"family: count\"\n\
\n\
-f | --stats    Print classified,unclassified,total numbers of sequences.\n\
\n\
If none of these is specified, -c is used.\n\
If multiple of these are specified, they are printed in the order as above.\n\
\n\n\
PROTEIN CLASSIFICATION OPTIONS:\n\
-P | --pthresh <n>      Protein threshold level. Allowed values:\n\
                            0   fixed threshold of 0.0\n\
                            2   less restrictive\n\
                            3   more restrictive\n\
                        Default is 2.\n\
"

#if MAIN_DNA
"\n\n\
DNA CLASSIFICATION OPTIONS:\n\
-l | --long             Use long read mode (default):\n\
                        Only accept certain ORFs (see -O below) and report all\n\
                        protein scores above the threshold (see -P above).\n\
\n\
-s | --short            Use short read mode:\n\
                        Accept all ORFS, report only maximum protein score (if\n\
                        above threshold).\n\
\n\
-O | --othresh <n>      ORF translation threshold level (only relevant in long\n\
                        read mode. Allowed values:\n\
                            0   accept all ORFs\n\
                            1   less restrictive\n\
                            2   more restrictive\n\
                        Default is 1.\n\
"
#endif
;
    fprintf(stderr, s, progname);
}

void
print_version(const char *progname)
{
    (void) progname;
    fprintf(stderr, "%s version 0.0\n", "uproc-beta");
}

enum args
{
    DBDIR, MODELDIR, INFILE,
    ARGC
};

int
parse_int(const char *arg, int *x)
{
    char *end;
    int tmp = strtol(arg, &end, 10);
    if (!*arg || *end) {
        return UPROC_FAILURE;
    }
    *x = tmp;
    return UPROC_SUCCESS;
}

int
main(int argc, char **argv)
{
    int res;
    size_t i, i_chunk = 0, i_buf = 0, n_seqs = 0;
    bool error = false, more_input;

    struct uproc_ecurve fwd, rev;
    struct uproc_substmat substmat;

    struct uproc_protclass protclass;
    enum uproc_pc_mode pc_mode = UPROC_PC_ALL;
    struct uproc_matrix prot_thresholds;
    void *prot_filter_arg = &prot_thresholds;
    int prot_thresh_num = 2;


#if MAIN_DNA
    struct uproc_dnaclass dnaclass;
    enum uproc_dc_mode dc_mode = UPROC_DC_ALL;
    struct uproc_matrix codon_scores, orf_thresholds;
    int orf_thresh_num = 1;
    bool short_read_mode = false;
#endif

    uproc_io_stream *stream;
    struct uproc_fasta_reader rd;

    struct buffer *in, *out;
    size_t chunk_size = get_chunk_size();

    int opt;
    int format = UPROC_STORAGE_MMAP;

    uproc_io_stream *out_stream = NULL;
    size_t unexplained = 0, *out_unexplained = NULL;
    uintmax_t counts[UPROC_FAMILY_MAX] = { 0 }, *out_counts = NULL;

#define SHORT_OPTS_PROT "hvpcfP:t:"
#if MAIN_DNA
#define SHORT_OPTS SHORT_OPTS_PROT "slO:"
#else
#define SHORT_OPTS SHORT_OPTS_PROT
#endif

#ifdef _GNU_SOURCE
    struct option long_opts[] = {
        { "help",       no_argument,        NULL, 'h' },
        { "version",    no_argument,        NULL, 'v' },
        { "preds",      no_argument,        NULL, 'p' },
        { "counts",     no_argument,        NULL, 'c' },
        { "stats",      no_argument,        NULL, 'f' },
        { "pthresh",    required_argument,  NULL, 'P' },
        { "threads",    required_argument,  NULL, 't' },
#if MAIN_DNA
        { "short",      no_argument,        NULL, 's' },
        { "long",       no_argument,        NULL, 'l' },
        { "othresh",    required_argument,  NULL, 'O' },
#endif
        { 0, 0, 0, 0 }
    };

    while ((opt = getopt_long(argc, argv, SHORT_OPTS, long_opts, NULL)) != -1)
#else
    while ((opt = getopt(argc, argv, SHORT_OPTS)) != -1)
#endif
    {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            case 'v':
                print_version(argv[0]);
                return EXIT_SUCCESS;
            case 'p':
                out_stream = uproc_stdout;
                break;
            case 'c':
                out_counts = counts;
                break;
            case 'f':
                out_unexplained = &unexplained;
                break;
            case 'P':
                {
                    int tmp = 42;
                    (void) parse_int(optarg, &tmp);
                    if (tmp != 0 && tmp != 2 && tmp != 3) {
                        fprintf(stderr, "protein threshold must be either 0, 2 or 3\n");
                        return EXIT_FAILURE;
                    }
                    if (tmp == 0) {
                        prot_filter_arg = NULL;
                    }
                    else {
                        prot_thresh_num = tmp;
                    }
                }
                break;
            case 't':
#if HAVE_OMP_H
                {
                    int res, tmp;
                    res = parse_int(optarg, &tmp);
                    if (res != UPROC_SUCCESS || tmp <= 0) {
                        fprintf(stderr, "-t argument must be a positive integer\n");
                        return EXIT_FAILURE;
                    }
                    omp_set_num_threads(tmp);
                }
#endif
                break;
#if MAIN_DNA
            case 's':
                short_read_mode = true;
                break;
            case 'l':
                short_read_mode = false;
                break;
            case 'O':
                {
                    int tmp = 42;
                    (void) parse_int(optarg, &tmp);
                    if (tmp != 0 && tmp != 1 && tmp != 2) {
                        fprintf(stderr, "-O argument must be either 0, 1 or 2\n");
                        return EXIT_FAILURE;
                    }
                    orf_thresh_num = tmp;
                }
                break;

#endif
            case '?':
                return EXIT_FAILURE;
        }
    }

    if (!out_stream && !out_counts && !out_unexplained) {
        out_counts = counts;
    }

    if (argc < optind + ARGC - 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

#if HAVE_MMAP
#define ECURVE_EXT "mmap"
#else
#define ECURVE_EXT "bin"
#endif

    res = uproc_storage_load(&fwd, format, UPROC_IO_GZIP, "%s/fwd." ECURVE_EXT,
                            argv[optind + DBDIR]);
    if (res != UPROC_SUCCESS) {
        fprintf(stderr, "failed to load forward ecurve\n");
        return EXIT_FAILURE;
    }

    res = uproc_storage_load(&rev, format, UPROC_IO_GZIP, "%s/rev." ECURVE_EXT,
                            argv[optind + DBDIR]);
    if (res != UPROC_SUCCESS) {
        fprintf(stderr, "failed to load reverse ecurve\n");
        return EXIT_FAILURE;
    }

    res = uproc_matrix_load(&prot_thresholds, UPROC_IO_GZIP,
                           "%s/prot_thresh_e%d", argv[optind + DBDIR], prot_thresh_num);
    if (res != UPROC_SUCCESS) {
        fprintf(stderr, "failed to load protein thresholds\n");
        return EXIT_FAILURE;
    }

    const char *model_dir = argv[optind + MODELDIR];
    res = uproc_substmat_load(&substmat, UPROC_IO_GZIP, "%s/substmat", model_dir);
    if (res != UPROC_SUCCESS) {
        fprintf(stderr, "failed to load protein thresholds\n");
        return EXIT_FAILURE;
    }

#if MAIN_DNA
    if (short_read_mode) {
        pc_mode = UPROC_PC_MAX;
        dc_mode = UPROC_DC_MAX;
    }
    else {
        pc_mode = UPROC_PC_ALL;
        dc_mode = UPROC_DC_ALL;
    }
    if (!short_read_mode && orf_thresh_num != 0) {
        res = uproc_matrix_load(&orf_thresholds, UPROC_IO_GZIP, "%s/orf_thresh_e%d", model_dir, orf_thresh_num);
        if (res != UPROC_SUCCESS) {
            fprintf(stderr, "failed to load ORF thresholds\n");
            return EXIT_FAILURE;
        }
        res = uproc_matrix_load(&codon_scores, UPROC_IO_GZIP, "%s/codon_scores", model_dir);
        if (res != UPROC_SUCCESS) {
            fprintf(stderr, "failed to load ORF thresholds\n");
            return EXIT_FAILURE;
        }
        uproc_dc_init(&dnaclass, dc_mode, &protclass, &codon_scores, orf_filter, &orf_thresholds);
    }
    else {
        uproc_dc_init(&dnaclass, dc_mode, &protclass, NULL, orf_filter, NULL);
    }
#endif
    uproc_pc_init(&protclass, pc_mode, &fwd, &rev, &substmat, prot_filter, prot_filter_arg);


    /* use stdin if no input file specified */
    if (argc < optind + ARGC) {
        argv[argc++] = "-";
    }

    for (; optind + INFILE < argc; optind++)
    {
        if (!strcmp(argv[optind + INFILE], "-")) {
            stream = uproc_stdin;
        }
        else {
            stream = uproc_io_open("r", UPROC_IO_GZIP, argv[optind + INFILE]);
            if (!stream) {
                fprintf(stderr, "error opening %s: ", argv[optind + INFILE]);
                perror("");
                return EXIT_FAILURE;
            }
        }

        uproc_fasta_reader_init(&rd, 8192);

        do {
            in = &buf[!i_buf];
            out = &buf[i_buf];
#pragma omp parallel private(i, res) shared(buf)
            {
#pragma omp for schedule(dynamic) nowait
                for (i = 0; i < out->n; i++) {
#if MAIN_DNA
                    res = uproc_dc_classify(&dnaclass, out->seq[i], &out->results[i]);
#else
                    res = uproc_pc_classify(&protclass, out->seq[i], &out->results[i]);
#endif
                }
            }
#pragma omp parallel private(res) shared(more_input)
            {
#pragma omp sections
                {
#pragma omp section
                    {
                        res = input_read(stream, &rd, in, chunk_size);
                        more_input = res > 0;
                        if (res < 0) {
                            uproc_perror("error reading input");
                            error = true;
                        }
                    }
#pragma omp section
                    {
                        if (i_chunk) {
                            output(out, out_stream, (i_chunk - 1) * chunk_size,
                                    out_counts, out_unexplained, &n_seqs);
                        }
                    }
                }
            }
            i_chunk += 1;
            i_buf ^= 1;
        } while (!error && more_input);
        uproc_io_close(stream);
        uproc_fasta_reader_free(&rd);
    }

    uproc_ecurve_destroy(&fwd);
    uproc_ecurve_destroy(&rev);
    uproc_matrix_destroy(&prot_thresholds);
#if MAIN_DNA
    if (!short_read_mode) {
        uproc_matrix_destroy(&codon_scores);
        uproc_matrix_destroy(&orf_thresholds);
    }
#endif

    buf_free(&buf[0]);
    buf_free(&buf[1]);

    if (out_unexplained) {
        uproc_io_printf(uproc_stdout, "%zu,", n_seqs - unexplained);
        uproc_io_printf(uproc_stdout, "%zu,", unexplained);
        uproc_io_printf(uproc_stdout, "%zu\n", n_seqs);
    }
    if (out_counts) {
        for (uproc_family i = 0; i < UPROC_FAMILY_MAX; i++) {
            if (out_counts[i]) {
                uproc_io_printf(uproc_stdout, "%" UPROC_FAMILY_PRI ": %ju\n",
                        i, out_counts[i]);
            }

        }
    }

    return res == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

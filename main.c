#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include <unistd.h>
#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

#include "ecurve.h"


#define MAX_CHUNK_SIZE (1 << 10)

struct buffer
{
    char *header[MAX_CHUNK_SIZE];
    size_t header_sz[MAX_CHUNK_SIZE];
    char *seq[MAX_CHUNK_SIZE];
    size_t seq_sz[MAX_CHUNK_SIZE];

#ifdef MAIN_DNA
    struct ec_dc_results results[MAX_CHUNK_SIZE];
#else
    struct ec_pc_results results[MAX_CHUNK_SIZE];
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
            return EC_FAILURE;
        }
        *dest = tmp;
        *dest_sz = len;
    }
    memcpy(*dest, src, len);
    return EC_SUCCESS;
}

int
input_read(ec_io_stream *stream, struct ec_fasta_reader *rd, struct buffer *buf,
           size_t chunk_size)
{
    int res = EC_SUCCESS;
    size_t i;

    for (i = 0; i < chunk_size; i++) {
        char *p1, *p2;
        size_t header_len;

        res = ec_fasta_read(stream, rd);
        if (res != EC_ITER_YIELD) {
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
        if (EC_ISERROR(res)) {
            break;
        }

        res = dup_str(&buf->seq[i], &buf->seq_sz[i], rd->seq, rd->seq_len);
        if (EC_ISERROR(res)) {
            break;
        }
    }
    buf->n = i;
    if (i && res == EC_ITER_STOP) {
        res = EC_SUCCESS;
    }

    for (; i < chunk_size; i++) {
        free(buf->seq[i]);
        buf->seq[i] = NULL;
    }
    return res;
}

bool
prot_filter(const char *seq, size_t len, ec_class cls, double score,
        void *opaque)
{
    static size_t rows, cols;
    struct ec_matrix *thresh = opaque;
    (void) seq, (void) cls;
    if (!thresh) {
        return score > 0.0;
    }
    if (!rows) {
        ec_matrix_dimensions(thresh, &rows, &cols);
    }
    if (len >= rows) {
        len = rows - 1;
    }
    return score >= ec_matrix_get(thresh, len, 0);
}

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

void
output(struct buffer *buf,
        ec_io_stream *pr_stream, size_t pr_seq_offset,
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
#ifdef MAIN_DNA
            struct ec_dc_pred *pred = &buf->results[i].preds[j];
            if (pr_stream) {
                ec_io_printf(pr_stream, "%zu,%s,%u,%" EC_CLASS_PRI ",%1.3f\n",
                        i + pr_seq_offset + 1,
                        buf->header[i],
                        pred->frame + 1,
                        pred->cls,
                        pred->score);
            }
#else
            struct ec_pc_pred *pred = &buf->results[i].preds[j];
            if (pr_stream) {
                ec_io_printf(pr_stream, "%zu,%s,%" EC_CLASS_PRI ",%1.3f\n",
                        i + pr_seq_offset + 1,
                        buf->header[i],
                        pred->cls,
                        pred->score);
            }
#endif
            if (counts) {
                counts[pred->cls] += 1;
            }
        }
    }
}

size_t
get_chunk_size(void)
{
    size_t sz;
    char *end, *value = getenv("EC_CHUNK_SIZE");
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
    fprintf(stderr, "%s ?\n", progname);
}

void
print_version(const char *progname)
{
    fprintf(stderr, "%s version 0.0\n", progname);
}

enum args
{
    FWD, REV, INFILE,
    ARGC
};

int
main(int argc, char **argv)
{
    int res;
    size_t i, i_chunk = 0, i_buf = 0, n_seqs = 0;
    bool more_input;

    struct ec_ecurve fwd, rev;
    struct ec_substmat substmat[EC_SUFFIX_LEN];
    struct ec_matrix prot_thresholds;

#if MAIN_DNA
    struct ec_dnaclass dnaclass;
    struct ec_orf_codonscores codon_scores;
    struct ec_matrix orf_thresholds;
    bool short_read_mode = false;
#endif
    struct ec_protclass protclass;

    ec_io_stream *stream;
    struct ec_fasta_reader rd;

    struct buffer *in, *out;
    size_t chunk_size = get_chunk_size();

    int opt;
    int format = EC_STORAGE_MMAP;

    ec_io_stream *out_stream = NULL;
    size_t unexplained = 0, *out_unexplained = NULL;
    uintmax_t counts[EC_CLASS_MAX] = { 0 }, *out_counts = NULL;

#define SHORT_OPTS_PROT "hvBMpcfP:S:"
#ifdef MAIN_DNA
#define SHORT_OPTS SHORT_OPTS_PROT "slC:O:"
#else
#define SHORT_OPTS SHORT_OPTS_PROT
#endif

    ec_pc_init(&protclass, EC_PC_ALL, &fwd, &rev, NULL, prot_filter, NULL);
#ifdef MAIN_DNA
    ec_dc_init(&dnaclass, EC_DC_ALL, &protclass, NULL, orf_filter, NULL);
#endif

#ifdef _GNU_SOURCE
    struct option long_opts[] = {
        { "help",       no_argument,        NULL, 'h' },
        { "version",    no_argument,        NULL, 'v' },
        { "binary",     no_argument,        NULL, 'B' },
        { "mmap",       no_argument,        NULL, 'M' },
        { "preds",      no_argument,        NULL, 'p' },
        { "counts",     no_argument,        NULL, 'c' },
        { "fsu",        no_argument,        NULL, 'f' },
        { "pthresh",    required_argument,  NULL, 'P' },
        { "substmat",   required_argument,  NULL, 'S' },
#ifdef MAIN_DNA
        { "short",      no_argument,        NULL, 's' },
        { "long",       no_argument,        NULL, 'l' },
        { "cscores",    required_argument,  NULL, 'C' },
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
            case 'B':
                format = EC_STORAGE_BINARY;
                break;
            case 'M':
                format = EC_STORAGE_MMAP;
                break;
            case 'p':
                out_stream = ec_stdout;
                break;
            case 'c':
                out_counts = counts;
                break;
            case 'f':
                out_unexplained = &unexplained;
                break;
            case 'S':
                res = ec_substmat_load_many(substmat, EC_SUFFIX_LEN, optarg, EC_IO_GZIP);
                if (res != EC_SUCCESS) {
                    fprintf(stderr, "failed to load %s\n", optarg);
                    return EXIT_FAILURE;
                }
                protclass.substmat = substmat;
                break;
            case 'P':
                res = ec_matrix_load_file(&prot_thresholds, optarg, EC_IO_GZIP);
                if (res != EC_SUCCESS) {
                    fprintf(stderr, "failed to load %s\n", optarg);
                    return EXIT_FAILURE;
                }
                protclass.filter_arg = &prot_thresholds;
                break;
#ifdef MAIN_DNA
            case 's':
                short_read_mode = true;
                break;
            case 'l':
                short_read_mode = false;
                break;
            case 'C':
                res = ec_orf_codonscores_load_file(&codon_scores, optarg, EC_IO_GZIP);
                if (res != EC_SUCCESS) {
                    fprintf(stderr, "failed to load %s\n", optarg);
                    return EXIT_FAILURE;
                }
                dnaclass.codon_scores = &codon_scores;
                break;
            case 'O':
                res = ec_matrix_load_file(&orf_thresholds, optarg, EC_IO_GZIP);
                if (res != EC_SUCCESS) {
                    fprintf(stderr, "failed to load %s\n", optarg);
                    return EXIT_FAILURE;
                }
                dnaclass.orf_filter_arg = &orf_thresholds;
                break;

#endif
            case '?':
                return EXIT_FAILURE;
        }
    }

    if (!out_stream && !out_counts && !out_unexplained) {
        out_stream = ec_stdout;
    }

    if (argc < optind + ARGC - 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    res = ec_storage_load(&fwd, argv[optind + FWD], format, EC_IO_GZIP);
    if (res != EC_SUCCESS) {
        fprintf(stderr, "failed to load forward ecurve\n");
        return EXIT_FAILURE;
    }

    res = ec_storage_load(&rev, argv[optind + REV], format, EC_IO_GZIP);
    if (res != EC_SUCCESS) {
        fprintf(stderr, "failed to load reverse ecurve\n");
        return EXIT_FAILURE;
    }

    /* use stdin if no input file specified */
    if (argc < optind + ARGC) {
        argv[argc++] = "-";
    }


#ifdef MAIN_DNA
    if (short_read_mode) {
        if (dnaclass.orf_filter_arg) {
            fprintf(stderr, "WARNING: short read mode ignores \"-O\"\n");
            dnaclass.orf_filter_arg = NULL;
        }
        protclass.mode = EC_PC_MAX;
        dnaclass.mode = EC_DC_MAX;
    }
    else {
        if (dnaclass.orf_filter_arg && !dnaclass.codon_scores) {
            fprintf(stderr, "ERROR: \"-O\" requires \"-C\"\n");
            return EXIT_FAILURE;
        }
        if (dnaclass.codon_scores && !dnaclass.orf_filter_arg) {
            fprintf(stderr, "WARNING: \"-C\" ignored if \"-O\" not specified\n");
            dnaclass.codon_scores = NULL;
        }
        protclass.mode = EC_PC_ALL;
        dnaclass.mode = EC_DC_ALL;
    }
#endif

    for (; optind + INFILE < argc; optind++)
    {
        if (!strcmp(argv[optind + INFILE], "-")) {
            stream = ec_stdin;
        }
        else {
            stream = ec_io_open(argv[optind + INFILE], "r", EC_IO_GZIP);
            if (!stream) {
                fprintf(stderr, "error opening %s: ", argv[optind + INFILE]);
                perror("");
                return EXIT_FAILURE;
            }
        }

        ec_fasta_reader_init(&rd, 8192);

        do {
            in = &buf[!i_buf];
            out = &buf[i_buf];
#pragma omp parallel private(i, res) shared(buf)
            {
#pragma omp for schedule(dynamic) nowait
                for (i = 0; i < out->n; i++) {
#ifdef MAIN_DNA
                    res = ec_dc_classify(&dnaclass, out->seq[i], &out->results[i]);
#else
                    res = ec_pc_classify(&protclass, out->seq[i], &out->results[i]);
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
                        more_input = !EC_ISERROR(res) && res != EC_ITER_STOP;
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
        } while (more_input);
        ec_io_close(stream);
        ec_fasta_reader_free(&rd);
    }

    ec_ecurve_destroy(&fwd);
    ec_ecurve_destroy(&rev);
    if (protclass.filter_arg) {
        ec_matrix_destroy(&prot_thresholds);
    }

#ifdef MAIN_DNA
    if (dnaclass.orf_filter_arg) {
        ec_matrix_destroy(dnaclass.orf_filter_arg);
    }
#endif

    buf_free(&buf[0]);
    buf_free(&buf[1]);

    if (out_unexplained) {
        ec_io_printf(ec_stdout, "%zu,", n_seqs - unexplained);
        ec_io_printf(ec_stdout, "%zu,", unexplained);
        ec_io_printf(ec_stdout, "%zu\n", n_seqs);
    }
    if (out_counts) {
        for (ec_class i = 0; i < EC_CLASS_MAX; i++) {
            if (out_counts[i]) {
                ec_io_printf(ec_stdout, "%" EC_CLASS_PRI ": %ju\n",
                        i, out_counts[i]);
            }

        }
    }

    return res == EC_ITER_STOP ? EXIT_SUCCESS : EXIT_FAILURE;
}

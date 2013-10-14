#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

#include "ecurve.h"


#define MAX_CHUNK_SIZE (1 << 10)

struct buffer
{
#ifdef MAIN_DNA
    ec_class *cls[MAX_CHUNK_SIZE][EC_ORF_FRAMES];
    double *score[MAX_CHUNK_SIZE][EC_ORF_FRAMES];
    size_t count[MAX_CHUNK_SIZE][EC_ORF_FRAMES], seq_len[MAX_CHUNK_SIZE][EC_ORF_FRAMES];
#else
    ec_class *cls[MAX_CHUNK_SIZE];
    double *score[MAX_CHUNK_SIZE];
    size_t count[MAX_CHUNK_SIZE], seq_len[MAX_CHUNK_SIZE];
#endif
    char *id[MAX_CHUNK_SIZE];
    size_t id_sz[MAX_CHUNK_SIZE];

    char *seq[MAX_CHUNK_SIZE];
    size_t seq_sz[MAX_CHUNK_SIZE];
    size_t n;
} buf[2];

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
        char *p;
        res = ec_fasta_read(stream, rd);
        if (res != EC_ITER_YIELD) {
            break;
        }

        p = strpbrk(rd->header, ", \f\n\r\t\v");
        if (p) {
            rd->header_len = p - rd->header + 1;
            *p = '\0';
        }

        res = dup_str(&buf->id[i], &buf->id_sz[i], rd->header, rd->header_len);
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

double
threshold(struct ec_matrix *thresh, size_t seq_len)
{
    static size_t rows, cols;
    if (!rows) {
        ec_matrix_dimensions(thresh, &rows, &cols);
    }
    if (seq_len >= rows) {
        seq_len = rows - 1;
    }
    return ec_matrix_get(thresh, seq_len, 0);
}

void
output(struct buffer *buf,
        struct ec_matrix *thresholds, unsigned orf_mode,
        ec_io_stream *pr_stream, size_t pr_seq_offset,
        struct ec_bst *counts,
        size_t *unexplained,
        size_t *total)
{
    size_t i, j, count;
    ec_class *cls;
    double *score, thresh;
    union ec_bst_key key;
    union ec_bst_data value;
    *total += buf->n;
    for (i = 0; i < buf->n; i++) {
        bool explained = false;
#ifdef MAIN_DNA
        for (unsigned frame = 0; frame < orf_mode; frame++) {
            count = buf->count[i][frame];
            cls = buf->cls[i][frame];
            score = buf->score[i][frame];
            thresh = threshold(thresholds, buf->seq_len[i][frame]);
#else
        (void) orf_mode;
        count = buf->count[i];
        cls = buf->cls[i];
        score = buf->score[i];
        thresh = threshold(thresholds, buf->seq_len[i]);
#endif
            for (j = 0; j < count; j++) {
                if (score[j] > thresh) {
                    explained = true;
                    if (counts) {
                        key.uint = cls[j];
                        value.uint = 0;
                        (void) ec_bst_get(counts, key, &value);
                        value.uint++;
                        ec_bst_update(counts, key, value);
                    }
                    if (pr_stream) {
#ifdef MAIN_DNA
                        ec_io_printf(pr_stream, "%zu,%s,%u,%" EC_CLASS_PRI ",%1.3f\n",
                                i + pr_seq_offset + 1,
                                buf->id[i],
                                frame + 1,
                                cls[j],
                                score[j]);
#else
                        ec_io_printf(pr_stream, "%zu,%s,%" EC_CLASS_PRI ",%1.3f\n",
                                i + pr_seq_offset + 1,
                                buf->id[i],
                                cls[j],
                                score[j]);
#endif
                    }
                }
            }
#ifdef MAIN_DNA
        }
#endif
        if (unexplained) {
            *unexplained += !explained;
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


int cb_walk_print(union ec_bst_key k, union ec_bst_data v, void *opaque)
{
    ec_io_stream *stream = opaque;
    ec_io_printf(stream, "%ju: %ju\n", k.uint, v.uint);
    return EC_SUCCESS;
}

enum opts
{
    FWD,
    REV,
    SUBSTMAT,
    SCORE_THRESHOLDS,
#ifdef MAIN_DNA
    CODON_SCORES,
    ORF_THRESHOLDS,
#endif
    INFILE,
    ARGC,
};

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

int
main(int argc, char **argv)
{
    int res;
    size_t i, i_chunk = 0, i_buf = 0, n_seqs = 0;
    bool more_input;

    struct ec_ecurve fwd, rev;
    struct ec_substmat substmat[EC_SUFFIX_LEN];
    struct ec_matrix score_thresholds;

#if MAIN_DNA
    struct ec_orf_codonscores codon_scores;
    struct ec_matrix orf_thresholds;
#endif

    ec_io_stream *stream;
    struct ec_fasta_reader rd;

    struct buffer *in, *out;
    size_t chunk_size = get_chunk_size();

    int opt;
    int format = EC_STORAGE_MMAP;
    unsigned orf_mode = EC_ORF_PER_STRAND;

    ec_io_stream *out_stream = NULL;
    size_t unexplained = 0, *out_unexplained = NULL;
    struct ec_bst count_tree, *out_counts = NULL;

#define SHORT_OPTS_PROT "hBPMpcf"
#ifdef MAIN_DNA
#define SHORT_OPTS SHORT_OPTS_PROT "0126"
#else
#define SHORT_OPTS SHORT_OPTS_PROT
#endif

#ifdef _GNU_SOURCE
    struct option long_opts[] = {
        { "help",   no_argument,        NULL, 'h' },
        { "binary", no_argument,        NULL, 'B' },
        { "plain",  no_argument,        NULL, 'P' },
        { "mmap",   no_argument,        NULL, 'M' },
        { "predictions", no_argument,   NULL, 'p' },
        { "counts", no_argument,        NULL, 'c' },
        { "fsu",    no_argument,        NULL, 'f' },
        { 0, 0, 0, 0 }
    };

    while ((opt = getopt_long(argc, argv, SHORT_OPTS, long_opts, NULL)) != -1)
#else
    while ((opt = getopt(argc, argv, SHORT_OPTS)) != -1)
#endif
    {
        switch (opt) {
            case 'B':
                format = EC_STORAGE_BINARY;
                break;
            case 'P':
                format = EC_STORAGE_PLAIN;
                break;
            case 'M':
                format = EC_STORAGE_MMAP;
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            case '0':
                orf_mode = EC_ORF_MAX;
                break;
            case '1':
                orf_mode = EC_ORF_ALL;
                break;
            case '2':
                orf_mode = EC_ORF_PER_STRAND;
                break;
            case '6':
                orf_mode = EC_ORF_PER_FRAME;
                break;
            case 'p':
                out_stream = ec_stdout;
                break;
            case 'c':
                out_counts = &count_tree;
                break;
            case 'f':
                out_unexplained = &unexplained;
                break;
            case 'v':
                print_version(argv[0]);
                return EXIT_SUCCESS;
            case '?':
                return EXIT_FAILURE;
        }
    }

    if (!out_stream && !out_counts && !out_unexplained) {
        out_stream = ec_stdout;
    }

    ec_bst_init(&count_tree, EC_BST_UINT);

    if (argc < optind + ARGC - 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    res = ec_substmat_load_many(substmat, EC_SUFFIX_LEN, argv[optind + SUBSTMAT], EC_IO_GZIP);
    if (res != EC_SUCCESS) {
        fprintf(stderr, "failed to load substitution matrices\n");
        return EXIT_FAILURE;
    }

    res = ec_matrix_load_file(&score_thresholds, argv[optind + SCORE_THRESHOLDS], EC_IO_GZIP);
    if (res != EC_SUCCESS) {
        fprintf(stderr, "failed to load score thresholds\n");
        return EXIT_FAILURE;
    }

#ifdef MAIN_DNA
    res = ec_orf_codonscores_load_file(&codon_scores, argv[optind + CODON_SCORES], EC_IO_GZIP);
    if (res != EC_SUCCESS) {
        fprintf(stderr, "failed to load codon scores\n");
        return EXIT_FAILURE;
    }

    res = ec_matrix_load_file(&orf_thresholds, argv[optind + ORF_THRESHOLDS], EC_IO_GZIP);
    if (res != EC_SUCCESS) {
        fprintf(stderr, "failed to load ORF thresholds\n");
        return EXIT_FAILURE;
    }
#endif

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
#pragma omp parallel private(i, res)
            {
#pragma omp for schedule(dynamic) nowait
                for (i = 0; i < out->n; i++) {
#ifdef MAIN_DNA
                    res = ec_classify_dna_all(out->seq[i], orf_mode,
                            &codon_scores, &orf_thresholds, substmat, &fwd,
                            &rev, out->count[i], out->cls[i], out->score[i],
                            out->seq_len[i]);
#else
                    res = ec_classify_protein_all(out->seq[i], substmat, &fwd,
                            &rev, &out->count[i], &out->cls[i], &out->score[i]);
                    out->seq_len[i] = strlen(out->seq[i]);
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
                            output(out, &score_thresholds, orf_mode,
                                    out_stream, (i_chunk - 1) * chunk_size,
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
    ec_matrix_destroy(&score_thresholds);

#ifdef MAIN_DNA
    ec_matrix_destroy(&orf_thresholds);
#endif

    for (i = 0; i < chunk_size; i++) {
#ifdef MAIN_DNA
        for (unsigned f = 0; f < orf_mode; f++) {
            free(buf[0].score[i][f]);
            free(buf[0].cls[i][f]);
            free(buf[1].score[i][f]);
            free(buf[1].cls[i][f]);
        }
#else
        free(buf[0].score[i]);
        free(buf[0].cls[i]);
        free(buf[1].score[i]);
        free(buf[1].cls[i]);
#endif
    }

    if (out_unexplained) {
        fprintf(stdout, "explained:   %zu\n", n_seqs - unexplained);
        fprintf(stdout, "unexplained: %zu\n", unexplained);
        fprintf(stdout, "total:       %zu\n", n_seqs);
    }
    if (out_counts) {
        ec_bst_walk(&count_tree, cb_walk_print, stdout);
        ec_bst_clear(&count_tree, NULL);
    }

    return res == EC_ITER_STOP ? EXIT_SUCCESS : EXIT_FAILURE;
}

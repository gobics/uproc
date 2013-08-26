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
dup_str(char **dest, size_t *dest_sz, const char *src)
{
    size_t len = strlen(src) + 1;
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
input_read(FILE *stream, struct ec_seqio_filter *filter, struct buffer *buf,
           size_t chunk_size)
{
    int res = EC_SUCCESS;
    size_t i;

    char *id = NULL, *seq = NULL;
    size_t id_sz, seq_sz;

    for (i = 0; i < chunk_size && res == EC_SUCCESS; i++) {
        res = ec_seqio_fasta_read(stream, filter, &id, &id_sz, NULL, NULL,
                &seq, &seq_sz);
        char *p;
        if (res != EC_SUCCESS) {
            break;
        }
        p = strpbrk(id, ", \f\n\r\t\v");
        if (p) {
            *p = '\0';
        }
        res = dup_str(&buf->id[i], &buf->id_sz[i], id);
        if (res != EC_SUCCESS) {
            break;
        }

        res = dup_str(&buf->seq[i], &buf->seq_sz[i], seq);
        if (res != EC_SUCCESS) {
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

    free(id);
    free(seq);
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
output_print(FILE *stream, struct buffer *buf, unsigned offset, struct ec_matrix *thresholds, unsigned orf_mode)
{
    size_t i, j, count;
    ec_class *cls;
    double *score, thresh;
    for (i = 0; i < buf->n; i++) {
#ifdef MAIN_DNA
        for (unsigned frame = 0; frame < orf_mode; frame++) {
            count = buf->count[i][frame];
            cls = buf->cls[i][frame];
            score = buf->score[i][frame];
            thresh = threshold(thresholds, buf->seq_len[i][frame]);
            for (j = 0; j < count; j++) {
                if (score[j] > thresh) {
                    fprintf(stream, "%zu,%s,%u,%" EC_CLASS_PRI ",%1.3f\n",
                            i + offset + 1,
                            buf->id[i],
                            frame + 1,
                            cls[j],
                            score[j]);
                }
            }
        }
#else
        (void) orf_mode;
        count = buf->count[i];
        cls = buf->cls[i];
        score = buf->score[i];
        thresh = threshold(thresholds, buf->seq_len[i]);
        for (j = 0; j < count; j++) {
            if (score[j] > thresh) {
                fprintf(stream, "%zu,%s,%" EC_CLASS_PRI ",%1.3f\n",
                        i + offset + 1,
                        buf->id[i],
                        cls[j],
                        score[j]);
            }
        }
#endif

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
    size_t i, i_chunk = 0, i_buf = 0;
    bool more_input;

    struct ec_ecurve fwd, rev;
    struct ec_substmat substmat[EC_SUFFIX_LEN];
    struct ec_matrix score_thresholds;

#if MAIN_DNA
    struct ec_orf_codonscores codon_scores;
    struct ec_matrix orf_thresholds;
#endif

    FILE *stream;
    struct ec_seqio_filter filter;

    struct buffer *in, *out;
    size_t chunk_size = get_chunk_size();

    int opt;
    int format = EC_STORAGE_MMAP;
    unsigned orf_mode = EC_ORF_PER_STRAND;

#define SHORT_OPTS_PROT "hBPM"
#ifdef MAIN_DNA
#define SHORT_OPTS SHORT_OPTS_PROT "126"
#else
#define SHORT_OPTS SHORT_OPTS_PROT
#endif

#ifdef _GNU_SOURCE
    struct option long_opts[] = {
        { "help",   no_argument,        NULL, 'h' },
        { "binary", no_argument,        NULL, 'B' },
        { "plain",  no_argument,        NULL, 'P' },
        { "mmap",   no_argument,        NULL, 'M' },
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
            case '1':
                orf_mode = EC_ORF_ALL;
                break;
            case '2':
                orf_mode = EC_ORF_PER_STRAND;
                break;
            case '6':
                orf_mode = EC_ORF_PER_FRAME;
                break;
            case 'v':
                print_version(argv[0]);
                return EXIT_SUCCESS;
            case '?':
                return EXIT_FAILURE;
        }
    }

    if (argc < optind + ARGC - 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    res = ec_substmat_load_many(substmat, EC_SUFFIX_LEN, argv[optind + SUBSTMAT]);
    if (res != EC_SUCCESS) {
        fprintf(stderr, "failed to load substitution matrices\n");
        return EXIT_FAILURE;
    }

    res = ec_matrix_load_file(&score_thresholds, argv[optind + SCORE_THRESHOLDS]);
    if (res != EC_SUCCESS) {
        fprintf(stderr, "failed to load score thresholds\n");
        return EXIT_FAILURE;
    }

#ifdef MAIN_DNA
    res = ec_orf_codonscores_load_file(&codon_scores, argv[optind + CODON_SCORES]);
    if (res != EC_SUCCESS) {
        fprintf(stderr, "failed to load codon scores\n");
        return EXIT_FAILURE;
    }

    res = ec_matrix_load_file(&orf_thresholds, argv[optind + ORF_THRESHOLDS]);
    if (res != EC_SUCCESS) {
        fprintf(stderr, "failed to load ORF thresholds\n");
        return EXIT_FAILURE;
    }
#endif

    res = ec_storage_load_file(&fwd, argv[optind + FWD], format);
    if (res != EC_SUCCESS) {
        fprintf(stderr, "failed to load forward ecurve\n");
        return EXIT_FAILURE;
    }
    res = ec_storage_load_file(&rev, argv[optind + REV], format);
    if (res != EC_SUCCESS) {
        fprintf(stderr, "failed to load reverse ecurve\n");
        return EXIT_FAILURE;
    }

    ec_seqio_filter_init(&filter, EC_SEQIO_F_RESET, EC_SEQIO_WARN,
            "ACGTURYSWKMBDHVN.-", NULL, NULL);

    if (argc == optind + ARGC) {
        stream = fopen(argv[optind + INFILE], "r");
        if (!stream) {
            perror("");
            return EXIT_FAILURE;
        }
    }
    else {
        stream = stdin;
    }

    do {
        in = &buf[!i_buf];
        out = &buf[i_buf];
#pragma omp parallel private(i, res)
        {
#pragma omp for schedule(dynamic) nowait
            for (i = 0; i < out->n; i++) {
#ifdef MAIN_DNA
                res = ec_classify_dna_all(out->seq[i], orf_mode, &codon_scores,
                        &orf_thresholds, substmat, &fwd, &rev,
                        out->count[i], out->cls[i], out->score[i], out->seq_len[i]);
#else
                res = ec_classify_protein_all(out->seq[i], substmat, &fwd, &rev,
                        &out->count[i], &out->cls[i], &out->score[i]);
                out->seq_len[i] = strlen(out->seq[i]);
#endif
            }
        }

#pragma omp parallel
        {
#pragma omp sections
            {
#pragma omp section
                {
                    res = input_read(stream, &filter, in, chunk_size);
                    more_input = res == EC_SUCCESS;
                }
#pragma omp section
                {
                    if (i_chunk) {
                        output_print(stdout, out, (i_chunk - 1) * chunk_size,
                                &score_thresholds, orf_mode);
                    }
                }
            }
        }
        i_chunk += 1;
        i_buf ^= 1;
    } while (more_input);

    ec_ecurve_destroy(&fwd);
    ec_ecurve_destroy(&rev);
    ec_matrix_destroy(&score_thresholds);
    fclose(stream);

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
    return res == EC_ITER_STOP ? EXIT_SUCCESS : EXIT_FAILURE;
}
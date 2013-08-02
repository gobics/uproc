#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "ecurve.h"


#define MAX_CHUNK_SIZE (1 << 12)

#define BAIL do { if (res != EC_SUCCESS) {                  \
    fprintf(stderr, "failure in line %d\n", __LINE__ - 1);   \
    exit(EXIT_FAILURE); } } while (0)

enum args {
    DISTMAT = 1,
#ifdef MAIN_DNA
    CODONSCORES,
    THRESHOLDS,
#endif
    FWD,
    REV,
    INFILE,
    ARGC,
};


struct input
{
    char *id[MAX_CHUNK_SIZE], *seq[MAX_CHUNK_SIZE];
    size_t id_sz[MAX_CHUNK_SIZE], seq_sz[MAX_CHUNK_SIZE];
    size_t n;
} in;

struct output
{
    ec_class cls[MAX_CHUNK_SIZE];
    double score[MAX_CHUNK_SIZE];
    size_t n;
} out;

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
input_read(FILE *stream, struct ec_seqio_filter *filter, struct input *buf,
           size_t chunk_size)
{
    int res = EC_SUCCESS;
    size_t i;

    char *id = NULL, *seq = NULL;
    size_t id_sz, seq_sz;

    for (i = 0; i < chunk_size && res == EC_SUCCESS; i++) {
        res = ec_seqio_fasta_read(stream, filter, &id, &id_sz, NULL, NULL,
                                  &seq, &seq_sz);
        if (res != EC_SUCCESS) {
            break;
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
        free(buf->id[i]);
        free(buf->seq[i]);
        buf->id[i] = buf->seq[i] = NULL;
    }

    free(id);
    free(seq);
    return res;
}

void
output_print(FILE *stream, struct output *buf, unsigned offset)
{
    size_t i;
    for (i = 0; i < buf->n; i++) {
        fprintf(stream, "%zu, %" EC_CLASS_PRI ", %1.3f\n",
                i + offset + 1, buf->cls[i], buf->score[i]);
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

int
main(int argc, char **argv)
{
    int res;
    size_t i, i_chunk = 0;
    bool more_input;

    struct ec_substmat substmat[EC_SUFFIX_LEN];
#ifdef MAIN_DNA
    struct ec_orf_codonscores codon_scores;
    struct ec_matrix thresholds;
#endif
    struct ec_ecurve fwd, rev;

    FILE *stream;
    struct ec_seqio_filter filter;

    size_t chunk_size = get_chunk_size();

    if (argc < ARGC - 1) {
        return EXIT_FAILURE;
    }

    res = ec_substmat_load_many(substmat, EC_SUFFIX_LEN, argv[DISTMAT]);
    BAIL;

#ifdef MAIN_DNA
    res = ec_orf_codonscores_load_file(&codon_scores, argv[CODONSCORES]);
    BAIL;

    res = ec_matrix_load_file(&thresholds, argv[THRESHOLDS]);
    BAIL;
#endif

    res = ec_mmap_map(&fwd, argv[FWD]);
    BAIL;
    res = ec_mmap_map(&rev, argv[REV]);
    BAIL;

    ec_seqio_filter_init(&filter, EC_SEQIO_F_RESET, EC_SEQIO_WARN,
                         "ACGTURYSWKMBDHVN.-", NULL, NULL);

    if (argc == ARGC) {
        stream = fopen(argv[INFILE], "r");
        if (!stream) {
            perror("");
            return EXIT_FAILURE;
        }
    }
    else {
        stream = stdin;
    }

    do {
#pragma omp parallel private(i, res)
        {
#pragma omp for schedule(dynamic) nowait
            for (i = 0; i < in.n; i++) {
                if (in.seq[i]) {
#ifdef MAIN_DNA
                    res = ec_classify_dna(in.seq[i], EC_ORF_ALL, &codon_scores,
                            &thresholds, substmat, &fwd, &rev,
                            &out.cls[i], &out.score[i]);
#else
                    res = ec_classify_protein(in.seq[i], substmat, &fwd, &rev,
                            &out.cls[i], &out.score[i]);
#endif
                    if (res != EC_SUCCESS) {
                        out.score[i] = -INFINITY;
                    }
                }
                else {
                    printf("%zu\n", i);

                }
            }
        }
        out.n = in.n;

#pragma omp parallel
        {
#pragma omp sections
            {
#pragma omp section
                {
                    res = input_read(stream, &filter, &in, chunk_size);
                    more_input = res == EC_SUCCESS;
                }
#pragma omp section
                {
                    if (i_chunk) {
                        output_print(stdout, &out, (i_chunk - 1) * chunk_size);
                    }
                }
            }
        }
        i_chunk += 1;
    } while (more_input);

    ec_mmap_unmap(&fwd);
    ec_mmap_unmap(&rev);
#ifdef MAIN_DNA
    ec_matrix_destroy(&thresholds);
#endif
    fclose(stream);
    return res == EC_ITER_STOP ? EXIT_SUCCESS : EXIT_FAILURE;
}

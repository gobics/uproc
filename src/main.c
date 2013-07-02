#include <stdio.h>
#include <stdlib.h>
#include "ecurve.h"

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

int main(int argc, char **argv)
{
    int res;
    struct ec_distmat distmat[EC_SUFFIX_LEN];
#ifdef MAIN_DNA
    struct ec_orf_codonscores codon_scores;
    struct ec_matrix thresholds;
#endif
    struct ec_ecurve fwd, rev;

    FILE *stream;
    struct ec_seqio_filter filter;
    char *id = NULL, *seq = NULL;
    size_t id_sz, seq_sz;
    ec_class cls;
    double score;

    if (argc < ARGC - 1) {
        return EXIT_FAILURE;
    }

    res = ec_distmat_load_many(distmat, EC_SUFFIX_LEN, argv[DISTMAT],
                               &ec_distmat_load_plain, NULL);
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

    while ((res = ec_seqio_fasta_read(stream, &filter,
                                      &id, &id_sz,
                                      NULL, NULL,
                                      &seq, &seq_sz)) == EC_SUCCESS)
    {
        printf("%.10s: ", id);
        fflush(stdout);
#ifdef MAIN_DNA
        res = ec_classify_dna(seq, EC_ORF_ALL, &codon_scores, &thresholds,
                              distmat, &fwd, &rev, &cls, &score);
#else
        res = ec_classify_protein(seq, distmat, &fwd, &rev, &cls, &score);
#endif
        if (res == EC_FAILURE) {
            break;
        }
        printf("%" EC_CLASS_PRI ", %6.3f\n", cls, score);
    }
    free(id);
    free(seq);
    ec_mmap_unmap(&fwd);
    ec_mmap_unmap(&rev);
#ifdef MAIN_DNA
    ec_matrix_destroy(&thresholds);
#endif
    fclose(stream);
    return res == EC_ITER_STOP ? EXIT_SUCCESS : EXIT_FAILURE;
}

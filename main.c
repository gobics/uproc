/* uproc-prot and uproc-dna
 * Classify DNA/RNA or protein sequences.
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

#if MAIN_DNA
#define PROGNAME "uproc-dna"
#else
#define PROGNAME "uproc-prot"
#endif
#include "uproc_opt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#if _OPENMP
#include <omp.h>
#endif

#include <uproc.h>

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
#if MAIN_DNA
        uproc_dc_results_free(&buf->results[i]);
#else
        uproc_pc_results_free(&buf->results[i]);
#endif
    }
}

int
dup_str(char **dest, size_t *dest_sz, const char *src, size_t len)
{
    len += 1;
    if (len > *dest_sz) {
        char *tmp = realloc(*dest, len);
        if (!tmp) {
            return -1;
        }
        *dest = tmp;
        *dest_sz = len;
    }
    memcpy(*dest, src, len);
    return 0;
}

int
input_read(uproc_io_stream *stream, struct uproc_fasta_reader *rd,
           struct buffer *buf, size_t chunk_size)
{
    int res = 0;
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
    if (res >= 0 && buf->n == chunk_size) {
        return 1;
    }
    return res;
}

bool
prot_filter(const char *seq, size_t len, uproc_family family, double score,
            void *opaque)
{
    static size_t rows, cols;
    uproc_matrix *thresh = opaque;
    (void) seq, (void) family;
    if (!thresh) {
        return score > UPROC_EPSILON;
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
    uproc_matrix *thresh = opaque;
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
output(struct buffer *buf, size_t pr_seq_offset, size_t *n_seqs,
       uproc_idmap *idmap, uproc_io_stream *pr_stream,
       uintmax_t *counts, size_t *unexplained)
{
    size_t i, j;
    *n_seqs += buf->n;
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
#else
                struct uproc_pc_pred *pred = &buf->results[i].preds[j];
#endif
            if (pr_stream) {
#if MAIN_DNA
                uproc_io_printf(pr_stream, "%zu,%s,%u,", i + pr_seq_offset + 1,
                                buf->header[i], pred->orf.frame + 1);
#else
                uproc_io_printf(pr_stream, "%zu,%s,", i + pr_seq_offset + 1,
                                buf->header[i]);
#endif
                if (idmap) {
                    uproc_io_printf(pr_stream, "%s,",
                                    uproc_idmap_str(idmap, pred->family));
                }
                else {
                    uproc_io_printf(pr_stream, "%" UPROC_FAMILY_PRI ",",
                                    pred->family);
                }
                uproc_io_printf(pr_stream, "%1.3f\n", pred->score);
            }
            if (counts) {
                counts[pred->family] += 1;
            }
        }
    }
}


struct count
{
    uproc_family fam;
    uintmax_t n;
};

int
compare_count(const void *p1, const void *p2)
{
    const struct count *c1 = p1, *c2 = p2;

    /* sort by n in descending order */
    if (c1->n > c2->n) {
        return -1;
    }
    else if (c1->n < c2->n) {
        return 1;
    }

    /* or fam in ascending */
    if (c1->fam < c2->fam) {
        return -1;
    }
    else if (c1->fam > c2->fam) {
        return 1;
    }
    return 0;
}

void
output_counts(uintmax_t *counts, uproc_idmap *idmap, uproc_io_stream *out_stream)
{
    struct count c[UPROC_FAMILY_MAX];
    uproc_family i, n = 0;
    for (i = 0; i < UPROC_FAMILY_MAX; i++) {
        if (counts[i]) {
            c[n].fam = i;
            c[n].n = counts[i];
            n++;
        }
    }

    qsort(c, n, sizeof *c, compare_count);

    for (i = 0; i < n; i++) {
        if (idmap) {
            uproc_io_printf(out_stream, "%s", uproc_idmap_str(idmap, c[i].fam));
        }
        else {
            uproc_io_printf(out_stream, "%" UPROC_FAMILY_PRI, c[i].fam);
        }
        uproc_io_printf(out_stream, ",%ju\n", c[i].n);
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
USAGE: %s [options] DBDIR MODELDIR [INPUTFILES]\n\
\n\
Classify "
#if MAIN_DNA
"DNA/RNA"
#else
"protein"
#endif
" sequences using the database in DBDIR and the model in MODELDIR.\n\
INPUTFILES can be zero or more files containing sequences in FASTA format.\n\
If no file is specified or the file name is -, sequences will be read from\n\
standard input.\n\
\n\
GENERAL OPTIONS:\n"
OPT("-h", "--help   ", " ") "\n\
    Print this message and exit.\n\
\n"
OPT("-v", "--version", " ") "\n\
    Print version and exit.\n\
\n"
#if _OPENMP
OPT("-t", "--threads", "N") "\n\
    Maximum number of threads to use.\n\
    This overrides the environment variable OMP_NUM_THREADS. If neither is\n\
    set, the number of available CPU cores is used.\n"
#endif
"\n\n\
OUTPUT OPTIONS:\n"
OPT("-p", "--preds", "") "\n\
    Print all classifications as CSV with the fields:\n\
        sequence number (starting with 1)\n\
        FASTA header up to the first whitespace\n"
#if MAIN_DNA
    "\
        frame number (1-6)\n"
#endif
    "\
        protein family\n\
        classificaton score\n\
\n"
OPT("-f", "--stats", "") "\n\
    Print \"classified,unclassified,total\" numbers of sequences.\n\
\n"
OPT("-c", "--counts", "") "\n\
    Print protein family and number of classifications (if greater than 0)\n\
    separated by a comma.\n\
\n"
"If none of the above is specified, -c is used.\n\
If multiple of them are specified, they are printed in the order as above.\n\
\n\n"
OPT("-o", "--output", "FILE") "\n\
    Print output to FILE instead of standard output.\n\
\n"
OPT("-n", "--numeric", "") "\n\
    If used with -p or -c, print the internal numeric representation of\n\
    the families instead of their names.\n\
\n\n\
PROTEIN CLASSIFICATION OPTIONS:\n"
OPT("-P", "--pthresh", "N") "\n\
    Protein threshold level. Allowed values:\n\
        0   fixed threshold of 0.0\n\
        2   less restrictive\n\
        3   more restrictive\n\
    Default is 3.\n\
"

#if MAIN_DNA
"\n\n\
DNA CLASSIFICATION OPTIONS:\n"
OPT("-l", "--long", " ") "\n\
    Use long read mode (default):\n\
    Only accept certain ORFs (see -O below) and report all protein scores\n\
    above the threshold (see -P above).\n\
\n"
OPT("-s", "--short", " ") "\n\
    Use short read mode:\n\
    Accept all ORFs, report only maximum protein score (if above threshold).\n\
\n"
OPT("-O", "--othresh", "N") "\n\
    ORF translation threshold level (only relevant in long read mode).\n\
    Allowed values:\n\
        0   accept all ORFs\n\
        1   less restrictive\n\
        2   more restrictive\n\
    Default is 2.\n\
"
#endif
;
    fprintf(stderr, s, progname);
}

enum args
{
    DBDIR, MODELDIR, INFILES,
    ARGC
};

int
main(int argc, char **argv)
{
    int res = -1;
    long long i;
    size_t i_chunk = 0, i_buf = 0, n_seqs = 0;
    bool error = false, more_input;

    uproc_ecurve *fwd, *rev;
    uproc_idmap *idmap = NULL;
    bool use_idmap = true;
    uproc_substmat *substmat;

    uproc_protclass *protclass;
    enum uproc_pc_mode pc_mode = UPROC_PC_ALL;
    uproc_matrix *prot_thresholds = NULL;
    int prot_thresh_num = 3;


#if MAIN_DNA
    uproc_dnaclass *dnaclass;
    enum uproc_dc_mode dc_mode = UPROC_DC_ALL;
    uproc_matrix *codon_scores = NULL, *orf_thresholds = NULL;
    int orf_thresh_num = 2;
    bool short_read_mode = false;
#endif

    uproc_io_stream *stream;
    struct uproc_fasta_reader rd;

    struct buffer *in, *out;
    size_t chunk_size = get_chunk_size();

    int opt;

    uproc_io_stream *out_stream = uproc_stdout;
    bool out_preds = false,
         out_stats = false,
         out_counts = false;

    size_t unexplained = 0;
    uintmax_t counts[UPROC_FAMILY_MAX] = { 0 };

#define SHORT_OPTS_PROT "hvpcfo:nP:t:"
#if MAIN_DNA
#define SHORT_OPTS SHORT_OPTS_PROT "slO:"
#else
#define SHORT_OPTS SHORT_OPTS_PROT
#endif

#if HAVE_GETOPT_LONG
    struct option long_opts[] = {
        { "help",       no_argument,        NULL, 'h' },
        { "version",    no_argument,        NULL, 'v' },
        { "preds",      no_argument,        NULL, 'p' },
        { "counts",     no_argument,        NULL, 'c' },
        { "stats",      no_argument,        NULL, 'f' },
        { "output",     required_argument,  NULL, 'o' },
        { "numeric",    no_argument,        NULL, 'n' },
        { "pthresh",    required_argument,  NULL, 'P' },
        { "threads",    required_argument,  NULL, 't' },
#if MAIN_DNA
        { "short",      no_argument,        NULL, 's' },
        { "long",       no_argument,        NULL, 'l' },
        { "othresh",    required_argument,  NULL, 'O' },
#endif
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
            case 'p':
                out_preds = true;
                break;
            case 'c':
                out_counts = true;
                break;
            case 'f':
                out_stats = true;
                break;
            case 'o':
                out_stream = uproc_io_open("w", UPROC_IO_STDIO, "%s", optarg);
                if (!out_stream) {
                    uproc_perror("can't open output file");
                    return EXIT_FAILURE;
                }
                break;
            case 'n':
                use_idmap = false;
                break;
            case 'P':
                {
                    int tmp = 42;
                    (void) parse_int(optarg, &tmp);
                    if (tmp != 0 && tmp != 2 && tmp != 3) {
                        fprintf(
                            stderr, "protein threshold must be 0, 2 or 3\n");
                        return EXIT_FAILURE;
                    }
                    prot_thresh_num = tmp;
                }
                break;
            case 't':
#if _OPENMP
                {
                    int res, tmp;
                    res = parse_int(optarg, &tmp);
                    if (res || tmp <= 0) {
                        fprintf(stderr, "-t requires a positive integer\n");
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
                        fprintf(stderr, "-O argument must be 0, 1 or 2\n");
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

    if (!out_counts && !out_preds && !out_stats) {
        out_counts = true;
    }

    if (argc < optind + ARGC - 1) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    fwd = uproc_ecurve_load(UPROC_STORAGE_BINARY, UPROC_IO_GZIP,
                            "%s/fwd.ecurve", argv[optind + DBDIR]);
    if (!fwd) {
        fprintf(stderr, "failed to load forward ecurve\n");
        return EXIT_FAILURE;
    }

    rev = uproc_ecurve_load(UPROC_STORAGE_BINARY, UPROC_IO_GZIP,
                            "%s/rev.ecurve", argv[optind + DBDIR]);
    if (!rev) {
        fprintf(stderr, "failed to load reverse ecurve\n");
        return EXIT_FAILURE;
    }

    if (use_idmap) {
        idmap = uproc_idmap_load(UPROC_IO_GZIP, "%s/idmap",
                                 argv[optind + DBDIR]);
        if (!idmap) {
            fprintf(stderr, "failed to load idmap\n");
            return EXIT_FAILURE;
        }
    }

    if (prot_thresh_num) {
        prot_thresholds = uproc_matrix_load(UPROC_IO_GZIP,
                                            "%s/prot_thresh_e%d",
                                            argv[optind + DBDIR],
                                            prot_thresh_num);
        if (!prot_thresholds) {
            fprintf(stderr, "failed to load protein thresholds\n");
            return EXIT_FAILURE;
        }
    }
    else {
        prot_thresholds = NULL;
    }

    const char *model_dir = argv[optind + MODELDIR];
    substmat = uproc_substmat_load(UPROC_IO_GZIP, "%s/substmat", model_dir);
    if (!substmat) {
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
#endif
    protclass = uproc_pc_create(pc_mode, fwd, rev, substmat, prot_filter,
                                prot_thresholds);
    if (!protclass) {
        uproc_perror("");
        return EXIT_FAILURE;
    }
#if MAIN_DNA
    if (!short_read_mode && orf_thresh_num != 0) {
        orf_thresholds = uproc_matrix_load(UPROC_IO_GZIP, "%s/orf_thresh_e%d",
                                           model_dir, orf_thresh_num);
        if (!orf_thresholds) {
            fprintf(stderr, "failed to load ORF thresholds\n");
            return EXIT_FAILURE;
        }
        codon_scores = uproc_matrix_load(UPROC_IO_GZIP, "%s/codon_scores",
                                         model_dir);
        if (!codon_scores) {
            fprintf(stderr, "failed to load ORF thresholds\n");
            return EXIT_FAILURE;
        }
        dnaclass = uproc_dc_create(dc_mode, protclass, codon_scores,
                                   orf_filter, orf_thresholds);
    }
    else {
        dnaclass = uproc_dc_create(dc_mode, protclass, NULL, orf_filter,
                                   NULL);
    }
#endif

    /* use stdin if no input file specified */
    if (argc < optind + ARGC) {
        argv[argc++] = "-";
    }

    for (; optind + INFILES < argc; optind++)
    {
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

        uproc_fasta_reader_init(&rd, 8192);

        do {
            in = &buf[!i_buf];
            out = &buf[i_buf];
#pragma omp parallel private(i, res) shared(buf)
            {
#pragma omp for schedule(dynamic) nowait
                for (i = 0; i < (long long)out->n; i++) {
#if MAIN_DNA
                    res = uproc_dc_classify(dnaclass, out->seq[i],
                                            &out->results[i]);
#else
                    res = uproc_pc_classify(protclass, out->seq[i],
                                            &out->results[i]);
#endif
                    if (res < 0) {
                        uproc_perror("error");
                        error = true;
                    }
                }
            }
            if (error) {
                break;
            }
#pragma omp parallel private(res) shared(more_input)
            {
#pragma omp sections
                {
#pragma omp section
                    {
                        res = input_read(stream, &rd, in, chunk_size);
                        more_input = res > 0 || in->n;
                        if (res < 0) {
                            uproc_perror("error reading input");
                            error = true;
                        }
                    }
#pragma omp section
                    {
                        if (i_chunk) {
                            output(out, (i_chunk - 1) * chunk_size, &n_seqs,
                                   use_idmap ? idmap : NULL,
                                   out_preds ? out_stream : NULL,
                                   out_counts ? counts : NULL,
                                   out_stats ? &unexplained : NULL);
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

    uproc_pc_destroy(protclass);
    uproc_ecurve_destroy(fwd);
    uproc_ecurve_destroy(rev);
    uproc_substmat_destroy(substmat);
    if (prot_thresholds) {
        uproc_matrix_destroy(prot_thresholds);
    }
#if MAIN_DNA
    uproc_dc_destroy(dnaclass);
    if (codon_scores) {
        uproc_matrix_destroy(codon_scores);
    }
    if (orf_thresholds) {
        uproc_matrix_destroy(orf_thresholds);
    }
#endif

    buf_free(&buf[0]);
    buf_free(&buf[1]);

    if (out_stats) {
        uproc_io_printf(out_stream, "%zu,", n_seqs - unexplained);
        uproc_io_printf(out_stream, "%zu,", unexplained);
        uproc_io_printf(out_stream, "%zu\n", n_seqs);
    }
    if (out_counts) {
        output_counts(counts, use_idmap ? idmap : NULL, out_stream);
    }

    return res == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

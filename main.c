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
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if _OPENMP
#include <omp.h>
#endif

#include <uproc.h>

#if MAIN_DNA
#define PROGNAME "uproc-dna"
#else
#define PROGNAME "uproc-prot"
#endif

#define PROT_THRESH_DEFAULT 3
#define ORF_THRESH_DEFAULT 2

#define NUM_THREADS_DEFAULT 8
#define CHUNK_SIZE_DEFAULT (1 << 10)
#define CHUNK_SIZE_MAX (1 << 14)

#if MAIN_DNA
#define clf uproc_dnaclass
#define clf_classify uproc_dnaclass_classify
#define clfresult uproc_dnaresult
#else
#define clf uproc_protclass
#define clf_classify uproc_protclass_classify
#define clfresult uproc_protresult
#endif

timeit t_in, t_out, t_clf, t_tot;

struct buffer
{
    struct uproc_sequence seqs[CHUNK_SIZE_MAX];
    uproc_list *results[CHUNK_SIZE_MAX];
    long long n;
} buf[2];

#if MAIN_DNA
static void
map_list_dnaresult_free(void *value, void *opaque)
{
    (void) opaque;
    uproc_dnaresult_free(value);
}
#endif

void
buffer_free(struct buffer *buf)
{
    for (size_t i = 0; i < CHUNK_SIZE_MAX; i++) {
        uproc_sequence_free(&buf->seqs[i]);
        if (buf->results[i]) {
#if MAIN_DNA
            uproc_list_map(buf->results[i], map_list_dnaresult_free, NULL);
#endif
            uproc_list_destroy(buf->results[i]);
        }
    }
}

/* chunk size to use. can be overwritten by setting the UPROC_CHUNK_SIZE
 * environemt variable (see determine_chunk_size())*/
long long chunk_size = CHUNK_SIZE_DEFAULT;

void
determine_chunk_size(void)
{
    size_t sz;
    char *end, *value = getenv("UPROC_CHUNK_SIZE");
    if (value) {
        sz = strtoll(value, &end, 10);
        if (!*end && sz > 0 && sz <= CHUNK_SIZE_MAX) {
            chunk_size = sz;
            return;
        }
    }
    chunk_size = CHUNK_SIZE_DEFAULT;
}


/* Classify the buffer contents */
void
buffer_classify(struct buffer *buf, clf *classifier)
{
    long long i;
#pragma omp parallel for private(i) shared(buf, classifier) schedule(static)
    for (i = 0; i < buf->n; i++) {
        clf_classify(classifier, buf->seqs[i].data, &buf->results[i]);
    }
}

/* Read sequences from seqit and store them in buf.
 *
 * Returns non-zero if at least one sequence was read.
 */
int
buffer_read(struct buffer *buf, uproc_seqiter *seqit)
{
    long long i;
    for (i = 0; i < chunk_size; i++) {
        struct uproc_sequence seq;
        int res = uproc_seqiter_next(seqit, &seq);
        if (res) {
            break;
        }
        trim_header(seq.header);
        uproc_sequence_free(&buf->seqs[i]);
        uproc_sequence_copy(&buf->seqs[i], &seq);
    }
    buf->n = i;
    return buf->n > 0;
}


void
print_result(uproc_io_stream *stream,
             unsigned long seq_num,
             const char *header, unsigned long seq_len,
             struct clfresult *result,
             uproc_idmap *idmap)
{
    uproc_io_printf(stream, "%lu,%s,%lu", seq_num, header, seq_len);
#if MAIN_DNA
    uproc_io_printf(stream, ",%u,%lu,%lu", result->orf.frame + 1,
                    (unsigned long)result->orf.start + 1,
                    (unsigned long)result->orf.length);
#endif
    if (idmap) {
        uproc_io_printf(stream, ",%s", uproc_idmap_str(idmap, result->family));
    }
    else {
        uproc_io_printf(stream, ",%" UPROC_FAMILY_PRI, result->family);
    }
    uproc_io_printf(stream, ",%1.3f", result->score);
    uproc_io_printf(stream, "\n");
}

/* Process (and maybe output) classification results */
void
buffer_process(struct buffer *buf,
               unsigned long *n_seqs, unsigned long *n_seqs_unexplained,
               unsigned long counts[UPROC_FAMILY_MAX + 1],
               uproc_io_stream *out_preds, uproc_idmap *idmap)
{
    for (long long i = 0; i < buf->n; i++) {
        uproc_list *results = buf->results[i];
        long n_results = uproc_list_size(results);
        *n_seqs += 1;
        if (!n_results) {
            *n_seqs_unexplained += 1;
            continue;
        }

        struct clfresult result;
        for (long k = 0; k < n_results; k++) {
            uproc_list_get(results, k, &result);
            counts[result.family] += 1;
            if (out_preds) {
                print_result(out_preds, *n_seqs, buf->seqs[i].header,
                             strlen(buf->seqs[i].data), &result, idmap);
            }
        }
    }
}


void
classify_file_mt(const char *path, clf *classifier,
                 unsigned long *n_seqs, unsigned long *n_seqs_unexplained,
                 unsigned long counts[UPROC_FAMILY_MAX + 1],
                 uproc_io_stream *out_preds, uproc_idmap *idmap)
{
    int more_input;
    uproc_io_stream *stream = open_read(path);
    uproc_seqiter *seqit = uproc_seqiter_create(stream);

    unsigned i_buf = 0;
    timeit_start(&t_tot);
    do {
        struct buffer *buf_in = &buf[i_buf], *buf_out = &buf[!i_buf];
#pragma omp parallel sections
        {
#pragma omp section
            {
                timeit_start(&t_in);
                more_input = buffer_read(buf_in, seqit);
                timeit_stop(&t_in);
            }
#pragma omp section
            {
                timeit_start(&t_clf);
                buffer_classify(buf_out, classifier);
                timeit_stop(&t_clf);
                timeit_start(&t_out);
                buffer_process(buf_out, n_seqs, n_seqs_unexplained, counts,
                               out_preds, idmap);
                timeit_stop(&t_out);
            }
        }
        i_buf ^= 1;
    } while (more_input);
    timeit_stop(&t_tot);
    uproc_seqiter_destroy(seqit);
}

void
classify_file(const char *path, clf *classifier,
              unsigned long *n_seqs, unsigned long *n_seqs_unexplained,
              unsigned long counts[UPROC_FAMILY_MAX + 1],
              uproc_io_stream *out_preds, uproc_idmap *idmap)
{
#if _OPENMP
    if (omp_get_max_threads() > 1) {
        return classify_file_mt(path, classifier, n_seqs, n_seqs_unexplained,
                                counts, out_preds, idmap);
    }
#endif
    timeit_start(&t_tot);
    uproc_io_stream *stream = open_read(path);
    uproc_seqiter *seqit = uproc_seqiter_create(stream);
    struct uproc_sequence seq;
    uproc_list *results = NULL;
    timeit_start(&t_in);
    while (!uproc_seqiter_next(seqit, &seq)) {
        timeit_stop(&t_in);
        trim_header(seq.header);

        timeit_start(&t_clf);
        clf_classify(classifier, seq.data, &results);
        timeit_stop(&t_clf);

        timeit_start(&t_out);
        long n_results = uproc_list_size(results);
        *n_seqs += 1;
        if (!n_results) {
            *n_seqs_unexplained += 1;
        }
        for (long i = 0; i < n_results; i++) {
            struct clfresult result;
            uproc_list_get(results, i, &result);
            counts[result.family] += 1;
            print_result(out_preds, *n_seqs, seq.header, strlen(seq.data), &result, idmap);
        }
        timeit_stop(&t_out);

        timeit_start(&t_in);
    }
    timeit_stop(&t_in);
    uproc_seqiter_destroy(seqit);
    timeit_stop(&t_tot);
}

struct count
{
    uproc_family fam;
    unsigned long n;
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
print_counts(uproc_io_stream *stream,
        unsigned long counts[UPROC_FAMILY_MAX + 1], uproc_idmap *idmap)
{
    struct count c[UPROC_FAMILY_MAX + 1];
    uproc_family i, n = 0;
    for (i = 0; i < UPROC_FAMILY_MAX + 1; i++) {
        if (counts[i]) {
            c[n].fam = i;
            c[n].n = counts[i];
            n++;
        }
    }

    qsort(c, n, sizeof *c, compare_count);

    for (i = 0; i < n; i++) {
        if (idmap) {
            uproc_io_printf(stream, "%s", uproc_idmap_str(idmap, c[i].fam));
        }
        else {
            uproc_io_printf(stream, "%" UPROC_FAMILY_PRI, c[i].fam);
        }
        uproc_io_printf(stream, ",%lu\n", c[i].n);
    }
}

void
print_usage(const char *progname)
{
    const char *s =
PROGNAME ", version " PACKAGE_VERSION "\n\
\n\
USAGE: %s [options] DBDIR MODELDIR [INPUTFILES]\n\
\n\
Classify "
#if MAIN_DNA
"DNA/RNA"
#else
"protein"
#endif
" sequences using the database in DBDIR and the model in MODELDIR.\n\
INPUTFILES can be zero or more files containing sequences in FASTA or FASTQ\n\
format (FASTQ qualities are ignored).\n\
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
    Maximum number of threads to use (default: " STR(NUM_THREADS_DEFAULT) ").\n"
#endif
"\n\n\
OUTPUT OPTIONS:\n"
OPT("-p", "--preds", "") "\n\
    Print all classifications as CSV with the fields:\n\
        sequence number (starting with 1)\n\
        sequence header up to the first whitespace\n\
        sequence length\n"
#if MAIN_DNA
    "\
        ORF frame number (1-6)\n\
        ORF index in the DNA sequence (starting from 1)\n\
        ORF length\n"
#endif
    "\
        predicted protein family\n\
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
OPT("-z", "--zoutput", "FILE") "\n\
    Write gzip compressed output to FILE (use \"-\" for standard output).\n\
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
    Default is " STR(PROT_THRESH_DEFAULT) ".\n\
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
    Default is " STR(ORF_THRESH_DEFAULT) ".\n\
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
    uproc_error_set_handler(errhandler_bail);

    uproc_io_stream *out_stream = uproc_stdout;

    determine_chunk_size();
#if _OPENMP
    omp_set_nested(1);
    omp_set_num_threads(NUM_THREADS_DEFAULT);
#endif

    /* output option flags */
    bool
        out_preds = false,      // -p
        out_counts = false,     // -c
        out_stats = false,      // -f
        out_numeric = false;    // -n

    int prot_thresh_level = PROT_THRESH_DEFAULT;    // -P
    int orf_thresh_level = ORF_THRESH_DEFAULT;      // -O

    bool short_read_mode = false;   // -s

    int opt;

#define SHORT_OPTS_PROT "hvVpcfo:z:nP:t:"
#if MAIN_DNA
#define SHORT_OPTS SHORT_OPTS_PROT "slO:"
#else
#define SHORT_OPTS SHORT_OPTS_PROT
#endif

#if HAVE_GETOPT_LONG
    struct option long_opts[] = {
        { "help",       no_argument,        NULL, 'h' },
        { "version",    no_argument,        NULL, 'v' },
        { "libversion", no_argument,        NULL, 'V' },
        { "preds",      no_argument,        NULL, 'p' },
        { "counts",     no_argument,        NULL, 'c' },
        { "stats",      no_argument,        NULL, 'f' },
        { "output",     required_argument,  NULL, 'o' },
        { "zoutput",    required_argument,  NULL, 'z' },
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
                print_version(PROGNAME);
                return EXIT_SUCCESS;
            case 'V':
                uproc_features_print(uproc_stderr);
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
                out_stream = open_write(optarg, UPROC_IO_STDIO);
                break;
            case 'z':
                out_stream = open_write(optarg, UPROC_IO_GZIP);
                break;
            case 'n':
                out_numeric = true;
                break;
            case 'P':
                if (parse_prot_thresh_level(optarg, &prot_thresh_level)) {
                    fprintf(stderr, "-P argument must be 0, 2 or 3\n");
                    return EXIT_FAILURE;
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
                if (parse_orf_thresh_level(optarg, &orf_thresh_level)) {

                    fprintf(stderr, "-O argument must be 0, 1 or 2\n");
                    return EXIT_FAILURE;
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

    struct model model;
    model_load(&model, argv[optind + MODELDIR], orf_thresh_level);

    struct database db;
    database_load(&db, argv[optind + DBDIR], prot_thresh_level,
                  UPROC_ECURVE_BINARY);

    uproc_protclass *pc;
    uproc_dnaclass *dc;
    clf *classifier;

    create_classifiers(&pc, &dc, &db, &model, short_read_mode);
#if MAIN_DNA
    classifier = dc;
#else
    classifier = pc;
#endif

    uproc_idmap *idmap = out_numeric ? NULL : db.idmap;

    /* use stdin if no input file specified */
    if (argc < optind + ARGC) {
        argv[argc++] = "-";
    }

    unsigned long n_seqs = 0, n_seqs_unexplained = 0;
    unsigned long counts[UPROC_FAMILY_MAX + 1] = { 0 };

    for (; optind + INFILES < argc; optind++)
    {
        classify_file(argv[optind + INFILES], classifier,
                      &n_seqs, &n_seqs_unexplained, counts,
                      out_preds ? out_stream : NULL,
                      idmap);
    }

    if (out_stats) {
        uproc_io_printf(out_stream, "%lu,", n_seqs - n_seqs_unexplained);
        uproc_io_printf(out_stream, "%lu,", n_seqs_unexplained);
        uproc_io_printf(out_stream, "%lu\n", n_seqs);
    }
    if (out_counts) {
        print_counts(out_stream, counts, idmap);
    }

    uproc_io_close(out_stream);

    uproc_protclass_destroy(pc);
    uproc_dnaclass_destroy(dc);
    model_free(&model);
    database_free(&db);
    buffer_free(&buf[0]);
    buffer_free(&buf[1]);

    timeit_print(&t_in,  "in ");
    timeit_print(&t_out, "out");
    timeit_print(&t_clf, "clf");
    timeit_print(&t_tot, "tot");

    return EXIT_SUCCESS;
}

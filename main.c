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

#include "ppopts.h"
#include "devmode.h"

#if MAIN_DNA
#define PROGNAME "uproc-dna"
#else
#define PROGNAME "uproc-prot"
#endif

// Options controlled by command line arguments.
int flag_prot_thresh_level_ = 3;
int flag_orf_thresh_level_ = 2;
int flag_num_threads_ = 8;
bool flag_dna_short_read_mode_ = false;
bool flag_output_counts_ = false;
bool flag_output_summary_ = false;
bool flag_output_predictions_ = false;
bool flag_output_mosaicwords_ = false;
const char *flag_output_format_ = NULL;

// Devmode flags
bool flag_print_times_ = false;
bool flag_fake_results_ = false;

uproc_io_stream *output_stream_ = NULL;
uproc_database *database_;
uproc_rank ranks_count_;
uproc_model *model_;

unsigned long (*counts_)[UPROC_CLASS_MAX + 1];
void counts_increment(uproc_rank rank, uproc_class class)
{
    if (!flag_output_counts_) {
        return;
    }
    if (class == UPROC_CLASS_INVALID) {
        return;
    }
    uproc_assert(rank < ranks_count_);
    if (!counts_) {
        counts_ = calloc(ranks_count_, sizeof *counts_);
        if (!counts_) {
            uproc_error(UPROC_ENOMEM);
        }
    }
    counts_[rank][class] += 1;
}

// available in both protein/dna and detailed/non-detailed
#define OUTFMT_ALL "nhlcrs"

#ifdef MAIN_DNA
#define OUTFMT_DNA "FIL"
#else
#define OUTFMT_DNA ""
#endif
#define OUTFMT_PREDICTIONS OUTFMT_ALL OUTFMT_DNA
#define OUTFMT_MATCHES OUTFMT_PREDICTIONS "wdiS"

enum {
    OUTFMT_SEQ_NUMBER = 'n',
    OUTFMT_SEQ_HEADER = 'h',
    OUTFMT_SEQ_LENGTH = 'l',
    OUTFMT_CLASS = 'c',
    OUTFMT_RANK = 'r',
    OUTFMT_SCORE = 's',
#ifdef MAIN_DNA
    OUTFMT_ORF_FRAME = 'F',
    OUTFMT_ORF_INDEX = 'I',
    OUTFMT_ORF_LENGTH = 'L',
#endif
    OUTFMT_MATCH_WORD = 'w',
    OUTFMT_MATCH_DIRECTION = 'd',
    OUTFMT_MATCH_INDEX = 'i',
    OUTFMT_MATCH_SCORE = 'S',
};

#if MAIN_DNA
#define clf uproc_dnaclass
#define clf_classify uproc_dnaclass_classify
#define clfresult uproc_dnaresult
#define PROTRESULT(result) (&(result)->protresult)
#else
#define clf uproc_protclass
#define clf_classify uproc_protclass_classify
#define clfresult uproc_protresult
#define PROTRESULT(result) (result)
#endif
clf *classifier_;

#define CHUNK_SIZE_DEFAULT (1 << 10)
#define CHUNK_SIZE_MAX (1 << 14)

timeit t_in, t_out, t_clf, t_tot;

struct buffer
{
    struct uproc_sequence seqs[CHUNK_SIZE_MAX];
    uproc_list *results[CHUNK_SIZE_MAX];
    long long n;
} buf[2];

static void map_list_result_free(void *value, void *opaque)
{
    (void)opaque;
#if MAIN_DNA
    uproc_dnaresult_free(value);
#else
    uproc_protresult_free(value);
#endif
}

void buffer_free(struct buffer *buf)
{
    for (size_t i = 0; i < CHUNK_SIZE_MAX; i++) {
        uproc_sequence_free(&buf->seqs[i]);
        if (buf->results[i]) {
            uproc_list_map(buf->results[i], map_list_result_free, NULL);
            uproc_list_destroy(buf->results[i]);
        }
    }
}

/* chunk size to use. can be overwritten by setting the UPROC_CHUNK_SIZE
 * environemt variable (see determine_chunk_size())*/
long long chunk_size = CHUNK_SIZE_DEFAULT;

void determine_chunk_size(void)
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
void buffer_classify(struct buffer *buf)
{
    long long i;
#pragma omp parallel for private(i) shared(buf, classifier_) schedule(static)
    for (i = 0; i < buf->n; i++) {
        clf_classify(classifier_, buf->seqs[i].data, &buf->results[i]);
    }
}

/* Read sequences from seqit and store them in buf.
 *
 * Returns non-zero if at least one sequence was read.
 */
int buffer_read(struct buffer *buf, uproc_seqiter *seqit)
{
    long long i;
    for (i = 0; i < chunk_size; i++) {
        struct uproc_sequence seq;
        int res = uproc_seqiter_next(seqit, &seq);
        if (res) {
            // either an error occured (-1) or EOF was reached (1)
            break;
        }
        trim_header(seq.header);
        uproc_sequence_free(&buf->seqs[i]);
        uproc_sequence_copy(&buf->seqs[i], &seq);
    }
    buf->n = i;
    return buf->n > 0;
}

/* Invoked if -p or -m is used. With -m it will call itself for each
 * element in the result->mosaicwords list */
void print_result_or_match(unsigned long seq_num, const char *header,
                           unsigned long seq_len,
                           const struct clfresult *result,
                           const struct uproc_mosaicword *mosaicword)
{
    const char *format = flag_output_format_;
    if (!format) {
        return;
    }
    const struct uproc_protresult *protresult = PROTRESULT(result);

    if (!mosaicword && flag_output_mosaicwords_) {
        uproc_assert(protresult->mosaicwords);
        for (long i = 0; i < uproc_list_size(protresult->mosaicwords); i++) {
            struct uproc_mosaicword mosaicword;
            uproc_list_get(protresult->mosaicwords, i, &mosaicword);
            print_result_or_match(seq_num, header, seq_len, result,
                                  &mosaicword);
        }
        return;
    }

    while (*format) {
        // increment here so we know if theres another item
        switch (*format++) {
            case OUTFMT_SEQ_NUMBER:
                uproc_io_printf(output_stream_, "%lu", seq_num);
                break;
            case OUTFMT_SEQ_HEADER:
                uproc_io_puts(header, output_stream_);
                break;
            case OUTFMT_SEQ_LENGTH:
                uproc_io_printf(output_stream_, "%lu", seq_len);
                break;
#if MAIN_DNA
            case OUTFMT_ORF_FRAME:
                uproc_io_printf(output_stream_, "%u", result->orf.frame + 1);
                break;
            case OUTFMT_ORF_INDEX:
                uproc_io_printf(output_stream_, "%lu",
                                (unsigned long)result->orf.start + 1);
                break;
            case OUTFMT_ORF_LENGTH:
                uproc_io_printf(output_stream_, "%lu",
                                (unsigned long)result->orf.length);
                break;
#endif
            case OUTFMT_CLASS: {
                uproc_idmap *idmap =
                    uproc_database_idmap(database_, protresult->rank);
                uproc_io_puts(uproc_idmap_str(idmap, protresult->class),
                              output_stream_);
            } break;
            case OUTFMT_RANK:
                uproc_io_printf(output_stream_, "%u", protresult->rank + 1);
                break;
            case OUTFMT_SCORE:
                uproc_io_printf(output_stream_, "%1.3f", protresult->score);
                break;
            case OUTFMT_MATCH_WORD: {
                char w[UPROC_WORD_LEN + 1] = "";
                uproc_word_to_string(w, &mosaicword->word,
                                     uproc_database_alphabet(database_));
                uproc_io_puts(w, output_stream_);
            } break;
            case OUTFMT_MATCH_DIRECTION: {
                char *s = mosaicword->dir == UPROC_ECURVE_FWD ? "fwd" : "rev";
                uproc_io_puts(s, output_stream_);
            } break;
            case OUTFMT_MATCH_INDEX:
                uproc_io_printf(output_stream_, "%lu",
                                (unsigned long)mosaicword->index + 1);
                break;
            case OUTFMT_MATCH_SCORE:
                uproc_io_printf(output_stream_, "%1.3f", mosaicword->score);
                break;

            default:
                uproc_assert_msg(false, "default case should not be reached");
        }
        if (*format) {
            uproc_io_putc(',', output_stream_);
        }
    }
    uproc_io_putc('\n', output_stream_);
}

void print_matches(unsigned long seq_num, const char *header,
                   unsigned long seq_len, const struct clfresult *result)
{
    const struct uproc_protresult *protresult = PROTRESULT(result);

    if (!protresult->mosaicwords) {
        fprintf(stderr, "programmer is daft, mosaicwords is NULL\n");
        exit(EXIT_FAILURE);
    }
}

/* Process (and maybe output) classification results */
void buffer_process(struct buffer *buf, unsigned long *n_seqs,
                    unsigned long *n_seqs_unexplained)
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
            if (flag_output_predictions_ || flag_output_mosaicwords_) {
                print_result_or_match(*n_seqs, buf->seqs[i].header,
                                      strlen(buf->seqs[i].data), &result, NULL);
            }
            struct uproc_protresult *p = PROTRESULT(&result);
            counts_increment(p->rank, p->class);
        }
    }
}

void classify_fake_results(unsigned long *n_seqs,
                           unsigned long *n_seqs_unexplained)
{
    uproc_list *results = fake_results();
    for (long i = 0, n = uproc_list_size(results); i < n; i++) {
        struct uproc_dnaresult result;
        uproc_list_get(results, i, &result);
#if MAIN_DNA
        struct clfresult r = result;
#else
        struct clfresult r = result.protresult;
#endif
        char s[32];
        snprintf(s, sizeof s, "fake sequence %li", i);
        print_result_or_match(*n_seqs, s, 22, &r, NULL);
        struct uproc_protresult *p = PROTRESULT(&r);
        counts_increment(p->rank, p->class);
        *n_seqs += 1;
        uproc_protresult_free(p);
    }
    *n_seqs_unexplained += 3;
    *n_seqs += 3;
    uproc_list_destroy(results);
}

void classify_file_mt(const char *path, unsigned long *n_seqs,
                      unsigned long *n_seqs_unexplained)
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
                buffer_classify(buf_out);
                timeit_stop(&t_clf);
                timeit_start(&t_out);
                buffer_process(buf_out, n_seqs, n_seqs_unexplained);
                timeit_stop(&t_out);
            }
        }
        i_buf ^= 1;
    } while (more_input);
    timeit_stop(&t_tot);
    uproc_seqiter_destroy(seqit);
    uproc_io_close(stream);
}

void classify_file(const char *path, unsigned long *n_seqs,
                   unsigned long *n_seqs_unexplained)
{
#if _OPENMP
    if (omp_get_max_threads() > 1) {
        return classify_file_mt(path, n_seqs, n_seqs_unexplained);
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
        clf_classify(classifier_, seq.data, &results);
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
            if (flag_output_predictions_ || flag_output_mosaicwords_) {
                print_result_or_match(*n_seqs, seq.header, strlen(seq.data),
                                      &result, NULL);
            }
            struct uproc_protresult *p = PROTRESULT(&result);
            counts_increment(p->rank, p->class);
        }
        timeit_stop(&t_out);

        timeit_start(&t_in);
    }
    timeit_stop(&t_in);
    uproc_seqiter_destroy(seqit);
    uproc_list_map(results, map_list_result_free, NULL);
    uproc_list_destroy(results);
    uproc_io_close(stream);
    timeit_stop(&t_tot);
}

struct count
{
    uproc_class class;
    unsigned long n;
};

int compare_count(const void *p1, const void *p2)
{
    const struct count *c1 = p1, *c2 = p2;

    /* sort by n in descending order */
    if (c1->n > c2->n) {
        return -1;
    } else if (c1->n < c2->n) {
        return 1;
    }

    /* or class in ascending */
    if (c1->class < c2->class) {
        return -1;
    } else if (c1->class > c2->class) {
        return 1;
    }
    return 0;
}

void counts_print(void)
{
    if (!counts_) {
        return;
    }
    for (uproc_rank rank = 0; rank < ranks_count_; rank++) {
        struct count c[UPROC_CLASS_MAX + 1];
        uproc_class i, n = 0;
        for (i = 0; i < UPROC_CLASS_MAX + 1; i++) {
            if (counts_[rank][i]) {
                c[n].class = i;
                c[n].n = counts_[rank][i];
                n++;
            }
        }

        qsort(c, n, sizeof *c, compare_count);

        for (i = 0; i < n; i++) {
            uproc_idmap *idmap = uproc_database_idmap(database_, rank);
            uproc_io_puts(uproc_idmap_str(idmap, c[i].class), output_stream_);
            uproc_io_printf(output_stream_, ",%lu\n", c[i].n);
        }
    }
}

void make_opts(struct ppopts *o, const char *progname)
{
#define O(...) ppopts_add(o, __VA_ARGS__)
    ppopts_add_text(o, PROGNAME ", version " UPROC_VERSION);
    ppopts_add_text(o, "USAGE: %s [options] DBDIR MODELDIR [INPUTFILES]",
                    progname);

    ppopts_add_text(
        o,
        "Classifies %s sequences using the database in DBDIR and the model in "
        "MODELDIR. INPUTFILES can be zero or more files containing sequences "
        "in FASTA or FASTQ format (FASTQ qualities are ignored). If no file "
        "is specified or the file name is - (\"dash\" or \"minus\"), "
        "sequences will be read from standard input.",
#if MAIN_DNA
        "DNA/RNA"
#else
        "protein"
#endif
        );

    ppopts_add_header(o, "GENERAL OPTIONS:");
    O('h', "help", "", "Print this message and exit.");
    O('v', "version", "", "Print version and exit.");
    O('V', "libversion", "", "Print libuproc version/features and exit.");

#if _OPENMP
    O('t', "threads", "N", "Maximum number of threads to use (default: %d).",
      flag_num_threads_);
#endif

    ppopts_add_header(o, "OUTPUT FORMAT:");
    O('p', "predictions", "FORMAT",
      "\
Print all classifications as CSV with the fields specified by the format\n\
string FORMAT. Available characters are:\n\
    n: sequence number (starting from 1)\n\
    h: sequence header up to the first whitespace\n\
    l: sequence length (this is a lowercase L)\n"
#if MAIN_DNA
      "\
    F: ORF frame number (1-6)\n\
    I: ORF index in the DNA sequence (starting from 1)\n\
    L: ORF length\n"
#endif
      "\
    c: predicted class\n\
    r: rank of predicted class (1 is the lowest, e.g. protein family)\n\
    s: classification score");

    O('m', "matches", "FORMAT",
      "\
Like -p, but additionally collect and print information about every matched\n\
word. In addition to the format characters of -p (see above), the following\n\
are recognized:\n\
    m: matched amino acid word\n\
    d: match direction (\"fwd\" or \"rev\")\n\
    i: position of the word in the protein sequence\n\
    S: sum of the scores of all positions in the word");

    O('f', "stats", "",
      "Print \"CLASSIFIED,UNCLASSIFIED,TOTAL\" sequence counts.");
    O('c', "counts", "",
      "Print \"CLASS,COUNT\" where COUNT is the number of classifications "
      "for CLASS");
    ppopts_add_text(
        o,
        "If none of the above is specified, -c is used. If multiple of them "
        "are specified, they are printed in the same order as above.");

    ppopts_add_header(o, "OUTPUT OPTIONS:");
    O('o', "output", "FILE",
      "Write output to FILE instead of standard output.");
    O('z', "zoutput", "FILE",
      "Write gzipped output to FILE (use - for standard output).");
    O('n', "numeric", "",
      "If used with -p or -c, print the internal numeric representation of "
      "the protein families instead of their names.");

    ppopts_add_header(o, "PROTEIN CLASSIFICATION OPTIONS:");
    O('P', "pthresh", "N",
      "\
Protein threshold level. Allowed values:\n\
    0   fixed threshold of 0.0\n\
    2   less restrictive\n\
    3   more restrictive\n\
Default is %d . ",
      flag_prot_thresh_level_);

#if MAIN_DNA
    ppopts_add_header(o, "DNA CLASSIFICATION OPTIONS:");
    O('l', "long", "",
      "Use long read mode (default): Only accept certain ORFs (see -O below) "
      "and report all protein scores above the threshold (see -P above).");
    O('s', "short", "",
      "Use short read mode: Accept all ORFs, report only maximum protein "
      "score (if above threshold).");
    O('O', "othresh", "N",
      "\
ORF translation threshold level (only relevant in long read mode).\n\
Allowed values:\n\
    0   accept all ORFs\n\
    1   less restrictive\n\
    2   more restrictive\n\
Default is %d.",
      flag_orf_thresh_level_);
#endif

#ifdef DEVMODE
    ppopts_add_header(o, "DEVELOPER FLAGS:");
#if HAVE_TIMEIT
    O('T', "time", "",
      "Print stats about elapsed wallclock time(s)");
#endif
    O('R', "fake-results", "",
      "Ignore input file(s), load some fake results instead.");
#endif
#undef O
}

enum nonopt_args { DBDIR, MODELDIR, INFILES, ARGC };

int main(int argc, char **argv)
{
    uproc_error_set_handler(errhandler_bail, NULL);

    determine_chunk_size();

    int opt;
    struct ppopts opts = PPOPTS_INITIALIZER;
    make_opts(&opts, argv[0]);
    while ((opt = ppopts_getopt(&opts, argc, argv)) != -1) {
        switch (opt) {
            case 'h':
                ppopts_print(&opts, stdout, 80, PPOPTS_DESC_ON_NEXT_LINE);
                exit(EXIT_SUCCESS);
            case 'v':
                print_version(PROGNAME);
                exit(EXIT_SUCCESS);
            case 'V':
                uproc_features_print(uproc_stdout);
                exit(EXIT_SUCCESS);
            case 'p':
                flag_output_predictions_ = true;
                flag_output_format_ = optarg;
                break;
            case 'm':
                flag_output_mosaicwords_ = true;
                flag_output_format_ = optarg;
                break;
            case 'c':
                flag_output_counts_ = true;
                break;
            case 'f':
                flag_output_summary_ = true;
                break;
            case 'o':
                output_stream_ = open_write(optarg, UPROC_IO_STDIO);
                break;
            case 'z':
                output_stream_ = open_write(optarg, UPROC_IO_GZIP);
                break;
            case 'P':
                if (parse_prot_thresh_level(optarg, &flag_prot_thresh_level_)) {
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
#else
                fputs("UProC was compiled without OpenMP; -t ignored\n",
                      stderr);
                exit(EXIT_FAILURE);
#endif
            break;
#if MAIN_DNA
            case 's':
                flag_dna_short_read_mode_ = true;
                break;
            case 'l':
                flag_dna_short_read_mode_ = false;
                break;
            case 'O':
                if (parse_orf_thresh_level(optarg, &flag_orf_thresh_level_)) {
                    fprintf(stderr, "-O argument must be 0, 1 or 2\n");
                    exit(EXIT_FAILURE);
                }
                break;

#endif

#ifdef DEVMODE
            case 'T':
                flag_print_times_ = true;
                break;
            case 'R':
                flag_fake_results_ = true;
                break;
#endif
            case '?':
                exit(EXIT_FAILURE);
        }
    }

#if _OPENMP
    omp_set_nested(1);
    omp_set_num_threads(flag_num_threads_);
#endif

    if (!output_stream_) {
        output_stream_ = uproc_stdout;
    }

    // if no output mode was selected, set a default
    if (!flag_output_predictions_ && !flag_output_mosaicwords_ &&
        !flag_output_counts_ && !flag_output_summary_) {
        flag_output_summary_ = true;
    }

    // verify that all format string characters are valid
    if (flag_output_predictions_ || flag_output_mosaicwords_) {
        const char *valid =
            flag_output_predictions_ ? OUTFMT_PREDICTIONS : OUTFMT_MATCHES;
        size_t spn = strspn(flag_output_format_, valid);
        if (spn != strlen(flag_output_format_)) {
            fprintf(stderr, "%s: invalid format string character -- '%c'\n",
                    argv[0], flag_output_format_[spn]);
            return EXIT_FAILURE;
        }
    }

    if (argc < optind + ARGC - 1) {
        ppopts_print(&opts, stdout, 80, PPOPTS_DESC_ON_NEXT_LINE);
        return EXIT_FAILURE;
    }

    model_ = uproc_model_load(argv[optind + MODELDIR], flag_orf_thresh_level_);
    if (!model_) {
        return EXIT_FAILURE;
    }

    database_ = uproc_database_load(argv[optind + DBDIR], NULL, NULL);
    if (!database_) {
        return EXIT_FAILURE;
    }
    uintmax_t tmp;
    uproc_database_metadata_get_uint(database_, "ranks", &tmp);
    ranks_count_ = tmp;

    uproc_protclass *pc;
    uproc_dnaclass *dc;

    create_classifiers(&pc, &dc, database_, model_, flag_prot_thresh_level_,
                       flag_dna_short_read_mode_, flag_output_mosaicwords_);
#if MAIN_DNA
    classifier_ = dc;
#else
    classifier_ = pc;
#endif

    unsigned long n_seqs = 0, n_seqs_unexplained = 0;

    if (flag_fake_results_) {
        classify_fake_results(&n_seqs, &n_seqs_unexplained);
    } else {
        for (; optind + INFILES < argc; optind++) {
            /* use stdin if no input file specified */
            if (argc < optind + ARGC) {
                argv[argc++] = "-";
            }

            classify_file(argv[optind + INFILES], &n_seqs, &n_seqs_unexplained);
        }
    }

    if (flag_output_summary_) {
        uproc_io_printf(output_stream_, "%lu,", n_seqs - n_seqs_unexplained);
        uproc_io_printf(output_stream_, "%lu,", n_seqs_unexplained);
        uproc_io_printf(output_stream_, "%lu\n", n_seqs);
    }
    if (flag_output_counts_) {
        counts_print();
    }

    uproc_io_close(output_stream_);

    uproc_protclass_destroy(pc);
    uproc_dnaclass_destroy(dc);
    uproc_model_destroy(model_);
    uproc_database_destroy(database_);
    buffer_free(&buf[0]);
    buffer_free(&buf[1]);

    if (flag_print_times_) {
        timeit_print(&t_in, "in ");
        timeit_print(&t_out, "out");
        timeit_print(&t_clf, "clf");
        timeit_print(&t_tot, "tot");
    }

    return EXIT_SUCCESS;
}

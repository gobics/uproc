/* Copyright 2014 Peter Meinicke, Robin Martinjak
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

/* uproc-detailed - detailed protein classification
 *
 * This program works similar to `uproc-prot` (see main.c), except that it
 * prints a lot more information for each classified sequence:
 *
 * For every classified protein family (with classification score above the
 * threshold), every word that was found in one of the ecurves is reported,
 * along with its POTENTIAL portion of total score. "Potential" means that the
 * word score is just the sum of all positional scores that are equal to the
 * maximum at that particular position in the sequence. If two (or more) words
 * contribute the same score at a certain position, the result is that the sum
 * of word scores is not equal to the actual classification score.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <uproc.h>

#include "ppopts.h"


#define PROGNAME "uproc-detailed"
#define PROT_THRESH_DEFAULT 3

uproc_alphabet *alpha;
uproc_idmap *idmap;
uproc_io_stream *out_stream;

/* Items received via the trace callback (with the uproc_word converted into
 * its string representation) */
struct match
{
    uproc_family family;
    size_t index;
    bool reverse;
    char word[UPROC_WORD_LEN + 1];
    double scores[UPROC_SUFFIX_LEN];
};


void
column_maxes(uproc_list *matches, double *maxes, size_t sz)
{
    struct match match;

    for (size_t i = 0; i < sz; i++) {
        maxes[i] = -INFINITY;
    }

    for (long i = 0, n = uproc_list_size(matches); i < n; i++) {
        uproc_list_get(matches, i, &match);
        /* if it is not a reverse match, the suffix scoring starts at an offset
         * of UPROC_PREFIX_LEN */
        if (!match.reverse) {
            match.index += UPROC_PREFIX_LEN;
        }

        for (size_t k = 0; k < sz; k++) {
        /* check if requested column is in the range */
            if (k >= match.index && k <= match.index + UPROC_SUFFIX_LEN) {
                size_t idx = k - match.index;
                if (match.scores[idx] > maxes[k]) {
                    maxes[k] = match.scores[idx];
                }
            }
        }
    }
}


/** "collec" matches into a list */
void
trace_cb(const struct uproc_word *word, uproc_family family, size_t index,
         bool reverse, const double *scores, void *opaque)
{
    int res;
    uproc_bst *t = opaque;
    uproc_list *list = NULL;
    union uproc_bst_key key;
    struct match match = { .family = family, .index = index, .reverse = reverse };
    memcpy(match.scores, scores, sizeof match.scores);
    uproc_word_to_string(match.word, word, alpha);

    key.uint = family;
    res = uproc_bst_get(t, key, &list);
    if (res == UPROC_BST_KEY_NOT_FOUND) {
        list = uproc_list_create(sizeof match);
        uproc_bst_insert(t, key, &list);
    }
    uproc_list_append(list, &match);
}


/** Print all matched words for one family */
void
output_details(unsigned long seq_num, const struct uproc_sequence *seq,
               uproc_family family, uproc_list *matches)
{
    size_t seq_len = strlen(seq->data);
    double *maxes = malloc(seq_len * sizeof *maxes);
    if (!maxes) {
        /* let the installed errorhandler exit the program */
        uproc_error(UPROC_ENOMEM);
    }
    column_maxes(matches, maxes, seq_len);

    struct match match;
    for (long i = 0, n = uproc_list_size(matches); i < n; i++) {
        uproc_list_get(matches, i, &match);
        if (!match.reverse) {
            match.index += UPROC_PREFIX_LEN;
        }

        /* calculate sum of this matches scores which "contributed" to the
         * total classification score. this is the sum of all scores that are
         * the respective columns maximum */
        double sum = 0.0;
        for (size_t k = 0; k < UPROC_SUFFIX_LEN; k++) {
            double s = match.scores[k];
            if (isfinite(s) && s == maxes[k + match.index]) {
                sum += match.scores[k];
            }
        }

        if (sum != 0.0) {
            uproc_io_printf(out_stream, "%lu,%s,", seq_num, seq->header);
            if (!idmap) {
                uproc_io_printf(out_stream, "%" UPROC_FAMILY_PRI ",", family);
            }
            else {
                uproc_io_printf(out_stream, "%s,", uproc_idmap_str(idmap, family));
            }
            uproc_io_printf(out_stream, "%s,", match.word);
            uproc_io_printf(out_stream, "%s,", match.reverse ? "rev" : "fwd");
            uproc_io_printf(out_stream, "%lu,%1.5f\n", (unsigned long)match.index, sum);
        }
    }
    free(maxes);
}


void
bst_map_list_destroy(union uproc_bst_key key, void *value, void *opaque)
{
    uproc_list **p = value;
    (void)key;
    (void)opaque;
    uproc_list_destroy(*p);
}


void
classify_detailed(uproc_protclass *pc, const struct uproc_sequence *seq)
{
    static unsigned long seq_num = 1;
    uproc_bst *match_lists = uproc_bst_create(UPROC_BST_UINT, sizeof (uproc_list*));
    uproc_protclass_set_trace(pc, trace_cb, match_lists);

    uproc_list *results = NULL;
    uproc_protclass_classify(pc, seq->data, &results);
    for (long i = 0, n = uproc_list_size(results); i < n; i++) {
        struct uproc_protresult result;
        uproc_list_get(results, i, &result);

        uproc_list *matches;
        union uproc_bst_key key = { .uint = result.family };
        if (uproc_bst_remove(match_lists, key, &matches)) {
            exit(EXIT_FAILURE);
        }
        output_details(seq_num, seq, result.family, matches);
        uproc_list_destroy(matches);
    }
    uproc_list_destroy(results);
    uproc_bst_map(match_lists, bst_map_list_destroy, NULL);
    uproc_bst_destroy(match_lists);
    seq_num++;
}


void
classify_file(uproc_io_stream *stream, uproc_protclass *pc)
{
    struct uproc_sequence seq = UPROC_SEQUENCE_INITIALIZER;
    uproc_seqiter *seqit = uproc_seqiter_create(stream);

    while (!uproc_seqiter_next(seqit, &seq)) {
        trim_header(seq.header);
        classify_detailed(pc, &seq);
    }
    uproc_seqiter_destroy(seqit);
}


void
make_opts(struct ppopts *o, const char *progname)
{
#define O(...) ppopts_add(o, __VA_ARGS__)
    ppopts_add_text(o, PROGNAME ", version " UPROC_VERSION);
    ppopts_add_text(o,
        "USAGE: %s [options] DBDIR MODELDIR [INPUTFILES]", progname);

    ppopts_add_header(o, "GENERAL OPTIONS:");
    O('h', "help",       "",    "Print this message and exit.");
    O('v', "version",    "",    "Print version and exit.");
    O('V', "libversion", "",    "Print libuproc version/features and exit.");

    ppopts_add_header(o, "OUTPUT OPTIONS:");
    O('o', "output", "FILE",
      "Write output to FILE instead of standard output.");
    O('z', "zoutput", "FILE",
      "Write gzipped output to FILE (use - for standard output).");
    O('n', "numeric", "",
      "Print the internal numeric representation of the protein families "
      "instead of their names.");

    ppopts_add_header(o, "PROTEIN CLASSIFICATION OPTIONS:");
    O('P', "pthresh", "N", "\
Protein threshold level. Allowed values:\n\
    0   fixed threshold of 0.0\n\
    2   less restrictive\n\
    3   more restrictive\n\
Default is %d . ", PROT_THRESH_DEFAULT);
#undef O
}


enum nonopt_args
{
    DBDIR, MODELDIR, INFILES,
    ARGC
};


int main(int argc, char **argv)
{
    uproc_error_set_handler(errhandler_bail);
    out_stream = uproc_stdout;

    bool use_idmap = true;
    int prot_thresh_level = PROT_THRESH_DEFAULT;

    int opt;
    struct ppopts opts = PPOPTS_INITIALIZER;
    make_opts(&opts, argv[0]);
    while ((opt = ppopts_getopt(&opts, argc, argv)) != -1) {
        switch (opt) {
            case 'h':
                ppopts_print(&opts, stderr, 80, PPOPTS_DESC_ON_NEXT_LINE);
                return EXIT_SUCCESS;
            case 'v':
                print_version("uproc-detailed");
                return EXIT_SUCCESS;
            case 'o':
                out_stream = uproc_io_open("w", UPROC_IO_STDIO, "%s", optarg);
                break;
            case 'z':
                out_stream = uproc_io_open("w", UPROC_IO_GZIP, "%s", optarg);
                break;
            case 'n':
                use_idmap = false;
                break;
            case 'P':
                {
                    int tmp = 42;
                    (void) parse_int(optarg, &tmp);
                    if (tmp != 0 && tmp != 2 && tmp != 3) {
                        fprintf(stderr, "-P argument must be 0, 2 or 3\n");
                        return EXIT_FAILURE;
                    }
                    prot_thresh_level = tmp;
                }
                break;
            case '?':
                return EXIT_FAILURE;
        }
    }

    if (argc < optind + ARGC - 1) {
        ppopts_print(&opts, stderr, 80, PPOPTS_DESC_ON_NEXT_LINE);
        return EXIT_FAILURE;
    }

    struct database db;
    struct model model;
    model_load(&model, argv[optind + MODELDIR], 0);
    database_load(&db, argv[optind + DBDIR], prot_thresh_level,
                  UPROC_ECURVE_BINARY);

    if (use_idmap) {
        idmap = db.idmap;
    }
    alpha = uproc_ecurve_alphabet(db.fwd);
    uproc_protclass *pc;
    create_classifiers(&pc, NULL, &db, &model, false);

    if (argc < optind + ARGC) {
        argv[argc++] = "-";
    }

    for (; optind + INFILES < argc; optind++) {
        uproc_io_stream *stream = open_read(argv[optind + INFILES]);
        classify_file(stream, pc);
        uproc_io_close(stream);
    }
    uproc_io_close(out_stream);
    uproc_protclass_destroy(pc);
    model_free(&model);
    database_free(&db);
    return EXIT_SUCCESS;
}

/* uproc-makedb
 * Create a new uproc database.
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
#include <stdbool.h>
#include <string.h>
#include <time.h>

#if _OPENMP
#include <omp.h>
#endif

#include <uproc.h>
#include "libuproc/ecurve_internal.h"
#include "ppopts.h"

#define PROGNAME "uproc-makedb"

bool flags_no_calib_ = false;
bool flags_no_purge_ = false;
bool flags_no_ecurves_ = false;
bool flags_no_substmat_ = false;
char *flags_ranks_file_ = NULL;

char *modeldir_;
char *sourcefile_;
char *destdir_;

char alphabet_str_[UPROC_ALPHABET_SIZE + 1];
uproc_alphabet *alphabet_;

// The newly created database
uproc_database *database_;

// Number of distinguished classification ranks
uproc_rank ranks_count_ = 1;

// Maps (uproc_class) -> (uproc_class[UPROC_RANKS_MAX])
// The keys are the class at the lowest rank and values the classes of all
// ranks. NULL iff ranks_count_ == 1.
uproc_dict *ranks_;

uproc_idmap *idmaps_[UPROC_RANKS_MAX];

struct ecurve_entry
{
    struct uproc_word word;
    uproc_class classes[UPROC_RANKS_MAX];
};

int ecurve_entry_cmp(const void *p1, const void *p2) {
    const struct ecurve_entry *e1 = p1, *e2 = p2;
    return uproc_word_cmp(&e1->word, &e2->word);
}

char *crop_first_word(char *s)
{
    char *p1, *p2;
    size_t len;
    p1 = s;
    while (isspace(*p1)) {
        p1++;
    }
    p2 = strpbrk(p1, ", \f\n\r\t\v");
    if (p2) {
        len = p2 - p1 + 1;
        *p2 = '\0';
    } else {
        len = strlen(p1);
    }
    memmove(s, p1, len + 1);
    return s;
}

char *reverse_string(char *s)
{
    char *p1, *p2;
    p1 = s;
    p2 = s + strlen(s) - 1;
    while (p2 > p1) {
        char tmp = *p2;
        *p2 = *p1;
        *p1 = tmp;
        p1++;
        p2--;
    }
    return s;
}

uproc_list *extract_uniques(uproc_io_stream *stream, uproc_amino first,
                            bool reverse)
{
    uproc_assert(alphabet_);
    struct uproc_sequence seq;
    size_t index;

    uproc_class classes[UPROC_RANKS_MAX] = {0};

    uproc_dict *words = uproc_dict_create(sizeof (struct uproc_word),
                                          sizeof classes);

    uproc_seqiter *rd = uproc_seqiter_create(stream);
    while (!uproc_seqiter_next(rd, &seq)) {
        uproc_worditer *iter;
        struct uproc_word fwd_word = UPROC_WORD_INITIALIZER,
                          rev_word = UPROC_WORD_INITIALIZER;

        crop_first_word(seq.header);
        classes[0] = uproc_idmap_class(idmaps_[0], seq.header);
        if (ranks_) {
            uproc_dict_get(ranks_, &classes[0], &classes);
        }

        if (reverse) {
            reverse_string(seq.data);
        }

        iter = uproc_worditer_create(seq.data, alphabet_);

        while (!uproc_worditer_next(iter, &index, &fwd_word, &rev_word)) {
            if (!uproc_word_startswith(&fwd_word, first)) {
                continue;
            }
            uproc_class tmp_classes[UPROC_RANKS_MAX];

            /* if the word was already present -> mark as duplicate for each
             * stored class that differs */
            if (uproc_dict_get(words, &fwd_word, &tmp_classes)) {
                for (int i = 0; i < ranks_count_; i++) {
                    uproc_class cls = classes[i], tmp_class = tmp_classes[i];
                    if (tmp_class != cls) {
                        tmp_classes[i] = UPROC_CLASS_INVALID;
                    }
                }
                uproc_dict_set(words, &fwd_word, &tmp_classes);
            } else {
                uproc_dict_set(words, &fwd_word, &classes);
            }
        }
        uproc_worditer_destroy(iter);
    }
    uproc_seqiter_destroy(rd);

    struct ecurve_entry entry = { UPROC_WORD_INITIALIZER, { 0 } };
    uproc_list *entries = uproc_list_create(sizeof entry);
    uproc_dictiter *iter = uproc_dictiter_create(words);
    while (!uproc_dictiter_next(iter, &entry.word, &entry.classes)) {
        uproc_list_append(entries, &entry);
    }
    uproc_dictiter_destroy(iter);
    uproc_dict_destroy(words);

    uproc_list_sort(entries, ecurve_entry_cmp);
    return entries;
}

void insert_entries(uproc_ecurve *ecurve, struct ecurve_entry *entries,
                    size_t entries_count)
{
    struct uproc_ecurve_suffixentry suffixentry;
    uproc_list *suffix_list = uproc_list_create(sizeof suffixentry);

    uproc_prefix current_prefix = entries[0].word.prefix;

    for (size_t i = 0; i < entries_count; i++) {
        if (entries[i].word.prefix != current_prefix) {
            uproc_ecurve_add_prefix(ecurve, current_prefix, suffix_list);
            uproc_list_clear(suffix_list);
            current_prefix = entries[i].word.prefix;
        }
        suffixentry.suffix = entries[i].word.suffix;
        memcpy(suffixentry.classes, entries[i].classes,
               sizeof suffixentry.classes);
        uproc_list_append(suffix_list, &suffixentry);
    }
    uproc_ecurve_add_prefix(ecurve, current_prefix, suffix_list);
    uproc_list_destroy(suffix_list);
}

uintmax_t djb2(const char *str) {
    uintmax_t hash = 5381;
    int c;

    while ((c = (unsigned char)*str++) != '\0') {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

// This is a pretty bad way to hash a file, but at least we're not pulling
// another dependency. Do not use something like this if you even need the
// tiniest bit of security.
uintmax_t hash_file(const char *path) {
    uproc_io_stream *stream = uproc_io_open("r", UPROC_IO_GZIP, path);
    char data[257] = { 0 };
    size_t n;
    uintmax_t hash = 0;
    do {
        n = uproc_io_read(data, 1, sizeof data - 1, stream);
        data[n] = '\0';
        hash ^= djb2(data);
    } while (n == sizeof data - 1);
    uproc_io_close(stream);
    return hash;
}

uproc_ecurve *build_ecurve(bool reverse)
{
    uproc_ecurve *ecurve = uproc_ecurve_create(alphabet_str_, ranks_count_, 0);

    progress(uproc_stderr, reverse ? "rev.ecurve" : "fwd.ecurve", -1.0);
    for (uproc_amino first = 0; first < UPROC_ALPHABET_SIZE; first++) {
        uproc_io_stream *stream =
            uproc_io_open("r", UPROC_IO_GZIP, sourcefile_);
        progress(uproc_stderr, NULL, first * 100 / UPROC_ALPHABET_SIZE);

        // list of struct ecurve_entry
        uproc_list *entries = extract_uniques(stream, first, reverse);
        uproc_io_close(stream);

        insert_entries(ecurve, uproc_list_get_ptr(entries),
                       uproc_list_size(entries));
        uproc_list_destroy(entries);
    }
    uproc_ecurve_finalize(ecurve);
    progress(uproc_stderr, NULL, 100);
    return ecurve;
}

void mark_singletons(uproc_ecurve *ec)
{
    uproc_assert(ec->suffix_count >= 1);

    enum { SINGLE, CLUSTER, BRIDGED, CROSSOVER };
    unsigned char *types = calloc(ec->suffix_count,
                                                     sizeof *types);
    if (!types) {
        uproc_error(UPROC_ENOMEM);
        uproc_perror("");
        exit(EXIT_FAILURE);
    }

    for (uproc_rank rank = 0; rank < ranks_count_; rank++) {
        for (size_t i = 0, n = ec->suffix_count; i < n - 1; i++) {
            unsigned char *t = &types[i];
#define cls(idx) uproc_ecurve_get_class(ec, i + (idx), rank)
            /* |AA..| */
            if (cls(0) == cls(1)) {
                t[0] = t[1] = CLUSTER;
                continue;
            }

            /* Continue unless we have |ABA.| */
            if (n < 2 || i == n - 2 || cls(0) != cls(2)) {
                continue;
            }

            /* B|ABA.| */
            if (t[1] == BRIDGED || t[1] == CROSSOVER) {
                t[0] = t[1] = t[2] = CROSSOVER;
            }
            /* |ABAB| */
            else if (n >= 3 && i < n - 3 &&
                     t[0] != CLUSTER &&
                     cls(1) == cls(3)) {
                t[0] = t[1] = t[2] = t[3] = CROSSOVER;
            }
            /* A|ABA.| or .|ABA.| */
            else {
                if (t[0] != CLUSTER && t[0] != CROSSOVER) {
                    t[0] = BRIDGED;
                }
                t[2] = BRIDGED;
            }
#undef cls
        }
        for (size_t i = 0; i < ec->suffix_count; i++) {
            if (types[i] == SINGLE || types[i] == CROSSOVER) {
                uproc_ecurve_set_class(ec, i, rank, UPROC_CLASS_INVALID);
            }
        }
    }
    free(types);
}

/* removes all entries that have no valid class on any rank */
uproc_ecurve *purge_ecurve(const uproc_ecurve *ecurve) {
    uproc_ecurve *new = uproc_ecurve_create(alphabet_str_, ranks_count_, 0);


    struct uproc_ecurve_suffixentry suffix_entry;
    uproc_list *suffix_list = uproc_list_create(sizeof suffix_entry);

    const struct uproc_ecurve_pfxtable *table = ecurve->prefixes;

    for (uproc_prefix p = 0; p <= UPROC_PREFIX_MAX; p++) {
        if (ECURVE_ISEDGE(table[p]) || table[p].count == 0) {
            continue;
        }

        uproc_list_clear(suffix_list);
        for (pfxtab_count i = 0; i < table[p].count; i++) {
            size_t s = table[p].first + i;
            suffix_entry.suffix = ecurve->suffixes[s];
            uproc_ecurve_get_classes(ecurve, s, suffix_entry.classes);

            for (uproc_rank rank = 0; rank < ranks_count_; rank++) {
                if (suffix_entry.classes[rank] != UPROC_CLASS_INVALID) {
                    uproc_list_append(suffix_list, &suffix_entry);
                    break;
                }
            }
        }

        if (uproc_list_size(suffix_list)) {
            uproc_ecurve_add_prefix(new, p, suffix_list);
        }
    }
    uproc_ecurve_finalize(new);

    return new;
}

void purge_db_ecurves(void) {
    uproc_ecurve *ec;

    progress(uproc_stderr, "purging ecurves", -1.0);
    mark_singletons(uproc_database_ecurve(database_, UPROC_ECURVE_FWD));
    ec = purge_ecurve(uproc_database_ecurve(database_, UPROC_ECURVE_FWD));
    uproc_database_set_ecurve(database_, UPROC_ECURVE_FWD, ec);
    progress(uproc_stderr, NULL, 50.0);

    mark_singletons(uproc_database_ecurve(database_, UPROC_ECURVE_REV));
    ec = purge_ecurve(uproc_database_ecurve(database_, UPROC_ECURVE_REV));
    uproc_database_set_ecurve(database_, UPROC_ECURVE_REV, ec);
    progress(uproc_stderr, NULL, 100.0);
}

// For calibration
#define SEQ_COUNT_MULTIPLIER 200000
#define POW_MIN 5
#define POW_MAX 11
#define POW_DIFF (POW_MAX - POW_MIN)
#define LEN_MAX (1 << POW_MAX)
#define INTERP_MIN 20
#define INTERP_MAX 5000

int choice(const uproc_matrix *p, size_t n)
{
    double sum, c;
    unsigned i;
    c = (double)rand() / RAND_MAX;
    for (sum = 0, i = 0; sum < c && i < n; ++i) {
        if (p) {
            sum += uproc_matrix_get(p, 0, i);
        } else {
            sum += 1.0 / n;
        }
    }
    return i - 1;
}

void randseq(char *buf, size_t len, const uproc_matrix *aa_probs)
{
    size_t i;
    static bool rand_seeded = false;
    if (!rand_seeded) {
        srand(time(NULL));
        rand_seeded = true;
    }
    for (i = 0; i < len; i++) {
        uproc_amino a = choice(aa_probs, UPROC_ALPHABET_SIZE);
        buf[i] = uproc_alphabet_amino_to_char(alphabet_, a);
    }
    buf[i] = '\0';
}

int double_cmp(const void *p1, const void *p2)
{
    double delta = *(const double *)p1 - *(const double *)p2;
    if (fabs(delta) < UPROC_EPSILON) {
        return 0;
    }
    /* we want to sort in descending order */
    return delta < 0.0 ? 1 : -1;
}

int csinterp(const double *xa, const double *ya, int m, const double *x,
             double *y, int n)
{
    int i, low, high, mid;
    double h, b, a, u[m], ya2[m];

    ya2[0] = ya2[m - 1] = u[0] = 0.0;

    for (i = 1; i < m - 1; i++) {
        a = (xa[i] - xa[i - 1]) / (xa[i + 1] - xa[i - 1]);
        b = a * ya2[i - 1] + 2.0;
        ya2[i] = (a - 1.0) / b;
        u[i] = (ya[i + 1] - ya[i]) / (xa[i + 1] - xa[i]) -
               (ya[i] - ya[i - 1]) / (xa[i] - xa[i - 1]);
        u[i] = (6.0 * u[i] / (xa[i + 1] - xa[i - 1]) - a * u[i - 1]) / b;
    }

    for (i = m - 1; i > 0; i--) {
        ya2[i - 1] *= ya2[i];
        ya2[i - 1] += u[i - 1];
    }

    low = 0;
    high = m - 1;

    for (i = 0; i < n; i++) {
        if (i && (xa[low] > x[i] || xa[high] < x[i])) {
            low = 0;
            high = m - 1;
        }

        while (high - low > 1) {
            mid = (high + low) / 2;
            if (xa[mid] > x[i]) {
                high = mid;
            } else {
                low = mid;
            }
        }

        h = xa[high] - xa[low];
        if (h == 0.0) {
            return UPROC_EINVAL;
        }

        a = (xa[high] - x[i]) / h;
        b = (x[i] - xa[low]) / h;
        y[i] = a * ya[low] + b * ya[high] +
               ((a * a * a - a) * ya2[low] + (b * b * b - b) * ya2[high]) *
                   (h * h) / 6.0;
    }
    return 0;
}

uproc_matrix *interpolate(double thresh[static POW_DIFF + 1])
{
    double xa[POW_DIFF + 1], x[INTERP_MAX], y[INTERP_MAX];
    for (long i = 0; i < POW_DIFF + 1; i++) {
        xa[i] = i;
    }

    for (long i = 0; i < INTERP_MAX; i++) {
        double xi = i;
        if (i < INTERP_MIN) {
            xi = INTERP_MIN;
        }
        x[i] = log2(xi) - POW_MIN;
    }

    csinterp(xa, thresh, POW_DIFF + 1, x, y, INTERP_MAX);
    return uproc_matrix_create(1, INTERP_MAX, y);
}

bool prot_filter(const char *seq, size_t len, uproc_class class, double score,
                 void *opaque)
{
    (void)seq;
    (void)len;
    (void)class;
    (void)opaque;
    return score > UPROC_EPSILON;
}

void calib(void)
{
    double thresh2[POW_DIFF + 1], thresh3[POW_DIFF + 1];

    uproc_substmat *substmat = uproc_database_substitution_matrix(database_);
    if (!substmat) {
        substmat = uproc_substmat_load(UPROC_IO_GZIP, "%s/substmat", modeldir_);
    }
    uproc_matrix *aa_probs =
        uproc_matrix_load(UPROC_IO_GZIP, "%s/aa_probs", modeldir_);

    uproc_ecurve *fwd = uproc_database_ecurve(database_, UPROC_ECURVE_FWD);
    uproc_ecurve *rev = uproc_database_ecurve(database_, UPROC_ECURVE_REV);

#if _OPENMP
    omp_set_num_threads(POW_MAX - POW_MIN + 1);
#endif
    double perc = 0.0;
    int power;
    progress(uproc_stderr, "calibrating", perc);
#pragma omp parallel private(power) shared(fwd, rev, substmat, alphabet_, \
                                           aa_probs, perc)
    {
#pragma omp for
        for (power = POW_MIN; power <= POW_MAX; power++) {
            char seq[LEN_MAX + 1];

            uproc_list *all_scores = uproc_list_create(sizeof(double));

            unsigned long i, seq_count;
            size_t seq_len;
            uproc_clf *pc;
            uproc_list *results = NULL;
            pc = uproc_clf_create_protein(UPROC_CLF_ALL, false, fwd, rev,
                                          substmat, prot_filter, NULL);

            seq_len = 1 << power;
            seq_count = (1 << (POW_MAX - power)) * SEQ_COUNT_MULTIPLIER;
            seq[seq_len] = '\0';

            for (i = 0; i < seq_count; i++) {
                if (i && i % (seq_count / 100) == 0) {
#pragma omp critical
                    {
                        perc += 100.0 / (POW_MAX - POW_MIN + 1) / 100.0;
                        progress(uproc_stderr, NULL, perc);
                    }
                }
                randseq(seq, seq_len, aa_probs);
                uproc_clf_classify(pc, seq, &results);
                for (long i = 0, n = uproc_list_size(results); i < n; i++) {
                    struct uproc_result result;
                    uproc_list_get(results, i, &result);
                    uproc_list_append(all_scores, &result.score);
                }
            }
            uproc_list_destroy(results);

            uproc_list_sort(all_scores, double_cmp);
#define MIN(a, b) ((a) < (b) ? (a) : (b))
            double tmp;
            unsigned long n = uproc_list_size(all_scores);
            uproc_list_get(all_scores, MIN(seq_count / 100, n - 1), &tmp);
            thresh2[power - POW_MIN] = tmp;
            uproc_list_get(all_scores, MIN(seq_count / 1000, n - 1), &tmp);
            thresh3[power - POW_MIN] = tmp;
            uproc_list_destroy(all_scores);
            uproc_clf_destroy(pc);
        }
    }
    progress(uproc_stderr, NULL, 100.0);

    uproc_database_set_protein_threshold(database_, 2, interpolate(thresh2));
    uproc_database_set_protein_threshold(database_, 3, interpolate(thresh3));
}

uproc_alphabet *load_alphabet(void)
{
    char alphabet[UPROC_ALPHABET_SIZE + 2];  // +2 for "\n\0"
    uproc_io_stream *stream =
        uproc_io_open("r", UPROC_IO_GZIP, "%s/alphabet", modeldir_);
    if (!uproc_io_gets(alphabet, sizeof alphabet, stream)) {
        uproc_error_msg(UPROC_FAILURE, "can not read alphabet from %s/alphabet",
                        modeldir_);
    }
    uproc_io_close(stream);
    alphabet[UPROC_ALPHABET_SIZE] = '\0';
    memcpy(alphabet_str_, alphabet, UPROC_ALPHABET_SIZE + 1);
    return uproc_alphabet_create(alphabet);
}

void load_ranks_mapping(const char *path)
{
    uproc_class classes[UPROC_RANKS_MAX];
    ranks_ = uproc_dict_create(sizeof classes[0], sizeof classes);

    char *line = NULL;
    size_t line_sz = 0;
    uproc_io_stream *f = uproc_io_open("r", UPROC_IO_GZIP, path);
    while (uproc_io_getline(&line, &line_sz, f) != -1) {
        char *tok = strtok(line, ",");
        if (!tok) {
            fprintf(stderr, "can't load ranks mapping from file %s\n", path);
            fprintf(stderr, ">%s<\n", line);
            exit(EXIT_FAILURE);
        }
        crop_first_word(tok);
        classes[0] = uproc_idmap_class(idmaps_[0], tok);
        for (uproc_class i = 1; i < UPROC_RANKS_MAX; i++) {
            tok = strtok(NULL, ",");
            if (tok) {
                crop_first_word(tok);
            }
            if (tok && *tok) {
                if (i + 1 > ranks_count_) {
                    ranks_count_ = i + 1;
                    uproc_assert(!idmaps_[i]);
                    idmaps_[i] = uproc_idmap_create();
                    uproc_database_set_idmap(database_, i, idmaps_[i]);
                }
                classes[i] = uproc_idmap_class(idmaps_[i], tok);
            } else {
                classes[i] = UPROC_CLASS_INVALID;
            }
        }
        uproc_dict_set(ranks_, &classes[0], &classes);
    }
    free(line);
    uproc_io_close(f);
}

void make_opts(struct ppopts *o, const char *progname)
{
#define O(...) ppopts_add(o, __VA_ARGS__)
    ppopts_add_text(o, PROGNAME ", version " UPROC_VERSION);
    ppopts_add_text(o, "USAGE: %s [options] MODELDIR SOURCEFILE DESTDIR",
                    progname);

    ppopts_add_text(
        o,
        "Builds a UProC database from the model in MODELDIR and a FASTA/FASTQ \
        formatted SOURCEFILE and stores it in DESTDIR"
#if !defined(HAVE_MKDIR) && !defined(HAVE__MKDIR)
        " (which must already exist)"
#endif
        ".");
    ppopts_add_header(o, "GENERAL OPTIONS:");
    O('h', "help", "", "Print this message and exit.");
    O('v', "version", "", "Print version and exit.");
    O('V', "libversion", "", "Print libuproc version/features and exit.");
    O('r', "ranks", "FILE", /* XXX */ "WRITE ME");
    O('E', "no-ecurves", "",
      "Do not build ecurves (SOURCEFILE will be ignored).");
    O('S', "no-substmat", "", "Do not train substitution matrix.");
    O('P', "no-purge", "",
      "Include words in the ecurves which don't match \
      to a distinct class on at least one rank.");
    O('C', "no-calib", "", "Do not calibrate protein scoring thresholds.");

#undef O
}

void progress_cb(double p, void *arg)
{
    (void) arg;
    progress(uproc_stderr, NULL, p);
}

// from makedb_train_substmat.c
uproc_substmat *train_substmat(const uproc_ecurve *fwd,
                               const uproc_ecurve *rev);

#ifndef TEST

int main(int argc, char **argv)
{
    uproc_error_set_handler(errhandler_bail, NULL);

    int opt;
    struct ppopts opts = PPOPTS_INITIALIZER;
    make_opts(&opts, argv[0]);
    while ((opt = ppopts_getopt(&opts, argc, argv)) != -1) {
        switch (opt) {
            case 'h':
                ppopts_print(&opts, stdout, 80, 0);
                return EXIT_SUCCESS;
            case 'v':
                print_version(PROGNAME);
                return EXIT_SUCCESS;
            case 'V':
                uproc_features_print(uproc_stdout);
                return EXIT_SUCCESS;
            case 'E':
                flags_no_ecurves_ = true;
                break;
            case 'S':
                flags_no_substmat_ = true;
                break;
            case 'C':
                flags_no_calib_ = true;
                break;
            case 'P':
                flags_no_purge_ = true;
                break;
            case 'r':
                flags_ranks_file_ = optarg;
                break;

            case '?':
                return EXIT_FAILURE;
        }
    }

    if (!uproc_features_lapack() && !flags_no_substmat_) {
        fprintf(stderr,
                "Can't train substmat without LAPACK support in libuproc. "
                "Use -S to skip this step\n");
        return EXIT_FAILURE;
    }

    enum nonopt_args { MODELDIR, SOURCEFILE, DESTDIR, ARGC };
    if (argc < optind + ARGC) {
        ppopts_print(&opts, stdout, 80, 0);
        return EXIT_FAILURE;
    }
    modeldir_ = argv[optind + MODELDIR];
    sourcefile_ = argv[optind + SOURCEFILE];
    destdir_ = argv[optind + DESTDIR];

    alphabet_ = load_alphabet();

    if (flags_no_ecurves_) {
        int which = UPROC_DATABASE_LOAD_ALL & ~UPROC_DATABASE_LOAD_PROT_THRESH;
        database_ = uproc_database_load_some(destdir_, UPROC_DATABASE_LATEST,
                                             which, NULL, NULL);
    } else {
        database_ = uproc_database_create();
        make_dir(destdir_);
        idmaps_[0] = uproc_idmap_create();
        uproc_database_set_idmap(database_, 0, idmaps_[0]);

        uintmax_t id = hash_file(sourcefile_);
        if (flags_ranks_file_) {
            load_ranks_mapping(flags_ranks_file_);
            id ^= hash_file(flags_ranks_file_);
        }

        char id_str[17] = "";
        snprintf(id_str, sizeof id_str, "%jx", id);
        uproc_database_metadata_set_str(database_, "id", id_str);

        uproc_database_set_ecurve(database_, UPROC_ECURVE_FWD,
                                  build_ecurve(false));
        uproc_database_set_ecurve(database_, UPROC_ECURVE_REV,
                                  build_ecurve(true));

        uproc_database_metadata_set_uint(database_, "ranks", ranks_count_);
        uproc_database_metadata_set_str(database_, "alphabet", alphabet_str_);

        time_t now = time(NULL);
        uproc_database_metadata_set_str(database_, "created", ctime(&now));
        uproc_database_metadata_set_str(database_, "version", UPROC_VERSION);
        uproc_database_metadata_set_str(database_, "inputfile", sourcefile_);
    }

    if (!flags_no_substmat_) {
        uproc_substmat *s = train_substmat(
            uproc_database_ecurve(database_, UPROC_ECURVE_FWD),
            uproc_database_ecurve(database_, UPROC_ECURVE_REV));
        uproc_database_set_substitution_matrix(database_, s);
    } else {
        uproc_substmat *s = uproc_substmat_load(UPROC_IO_GZIP, "%s/substmat",
                                                modeldir_);
        uproc_database_set_substitution_matrix(database_, s);
    }

    if (!flags_no_purge_) {
        purge_db_ecurves();
    } else {
        // if DB is "purged", this is done in purge_db_ecurves.
        mark_singletons(uproc_database_ecurve(database_, UPROC_ECURVE_FWD));
        mark_singletons(uproc_database_ecurve(database_, UPROC_ECURVE_REV));
    }

    if (!flags_no_calib_) {
        calib();
    }

    if (flags_no_ecurves_) {
        // The ecurves are already mmapped from disk
        uproc_database_set_ecurve(database_, UPROC_ECURVE_FWD, NULL);
        uproc_database_set_ecurve(database_, UPROC_ECURVE_REV, NULL);
    }

    progress(uproc_stderr, "Storing database", -1);
    uproc_database_store(database_, destdir_, UPROC_DATABASE_LATEST,
                         progress_cb, NULL);
    progress(uproc_stderr, NULL, 100.0);
    return EXIT_SUCCESS;
}

#else


#define ELEMENTS(x) (sizeof(x) / sizeof(x)[0])
uproc_alphabet *alpha_;

struct entry {
    char w[UPROC_WORD_LEN + 2];
    uproc_class cls;
};

void add_entries(struct uproc_ecurve_s *ec, struct entry *entries, size_t n)
{
    ec->ranks_count = 1;

    struct uproc_ecurve_suffixentry suffix_entry = {0};
    uproc_list *suffixes = uproc_list_create(sizeof suffix_entry);

    for (size_t i = 0; i < n; i++) {
        struct uproc_word w = { 0, 0 };
        uproc_word_from_string(&w, entries[i].w, alpha_);
        suffix_entry.suffix = w.suffix;
        suffix_entry.classes[0] = entries[i].cls;
        uproc_list_append(suffixes, &suffix_entry);
    }

    uproc_ecurve_add_prefix(ec, 1337, suffixes);
    uproc_ecurve_finalize(ec);
}

void test_mark_singletons(void)
{
    struct {
        const char *name;
        size_t n;
        struct entry *entries;
        uproc_class *expected_classes;
    } tests[] = {
        {
            .name = "two clusters, one singleton",
            .n = 5,
            .entries = (struct entry[]) {
                {"PPPPPPAAAAAAAAAAAA", 0},
                {"PPPPPPAAAAAAAAAAAG", 0},
                {"PPPPPPGAAAAAAAAAGG", 1},
                {"PPPPPPGAAAAAAAAAGG", 0},
                {"PPPPPPGAAAAAAAAGGG", 0},
            },
            .expected_classes = (uproc_class[]) {
                0, 0, UPROC_CLASS_INVALID, 0, 0,
            },
        },
        {
            .name = "cluster, singleton, bridged, singleton",
            .n = 5,
            .entries = (struct entry[]) {
                {"PPPPPPAAAAAAAAAAAA", 0},
                {"PPPPPPAAAAAAAAAAAG", 0},
                {"PPPPPPGAAAAAAAAAGG", 1},
                {"PPPPPPGAAAAAAAAAGG", 0},
                {"PPPPPPGAAAAAAAAGGG", 2},
            },
            .expected_classes = (uproc_class[]) {
                0, 0, UPROC_CLASS_INVALID, 0, UPROC_CLASS_INVALID,
            },
        },
        {
            .name = "2x cluster, 2x crossover, 2x cluster",
            .n = 6,
            .entries = (struct entry[]) {
                {"PPPPPPAAAAAAAAAAAA", 0},
                {"PPPPPPAAAAAAAAAAAG", 0},
                {"PPPPPPGAAAAAAAAAGG", 1},
                {"PPPPPPGAAAAAAAAAGG", 0},
                {"PPPPPPGAAAAAAAAGGG", 1},
                {"PPPPPPGAAAAAAAGGGG", 1},
            },
            .expected_classes = (uproc_class[]) {
                0, 0, UPROC_CLASS_INVALID, UPROC_CLASS_INVALID, 1, 1,
            },
        },
    };

    for (size_t t = 0, n = ELEMENTS(tests); t < n; t++) {
        uproc_ecurve *ec = uproc_ecurve_create(alphabet_str_, 1, 0);
        add_entries(ec, tests[t].entries, tests[t].n);
        mark_singletons(ec);

        for (size_t i = 0; i < tests[t].n; i++) {
            uproc_class got = ec->classes[i],
                        want = tests[t].expected_classes[i];
            if (got != want) {
                fprintf(stderr,
                        "%s: test \"%s\", entry %zu: got %ju, want %ju\n",
                        __func__, tests[t].name, i, (uintmax_t)got,
                        (uintmax_t)want);
                exit(EXIT_FAILURE);
            }
        }
        uproc_ecurve_destroy(ec);
    }
}

void test_purge(void)
{
    struct entry entries_fwd[] = {
        { "PPPPPPAAAAAAAAAAAA", 0 },
        { "PPPPPPAAAAAAAAAAAG", 0 },
        { "PPPPPPGAAAAAAAAAGG", 1 },
        { "PPPPPPGAAAAAAAAAGG", 1 },
        { "PPPPPPGAAAAAAAAGGG", 1 },
    };
    struct entry entries_rev[] = {
        { "PPPPPPSSSSSSSSSSSS", 0 },
        { "PPPPPPSSSSSSSSSSSG", 0 },
        { "PPPPPPGSSSSSSSSSGG", UPROC_CLASS_INVALID },
        { "PPPPPPGSSSSSSSSSGG", UPROC_CLASS_INVALID },
        { "PPPPPPGSSSSSSSSSGG", UPROC_CLASS_INVALID },
        { "PPPPPPGSSSSSSSSSGG", 1 },
        { "PPPPPPGTTTTTTTTGGG", 1 },
    };

    uproc_ecurve *fwd = uproc_ecurve_create(alphabet_str_, 1, 0),
                 *rev = uproc_ecurve_create(alphabet_str_, 1, 0);

#define ADD_ENTRIES(ec, entries) add_entries((ec), (entries), ELEMENTS((entries)))
    ADD_ENTRIES(fwd, entries_fwd);
    ADD_ENTRIES(rev, entries_rev);
#undef ADD_ENTRIES

    uproc_ecurve *ec;
    uproc_assert(fwd->suffix_count == 5);
    ec = purge_ecurve(fwd);
    uproc_assert(ec->suffix_count == 5);

    uproc_assert(rev->suffix_count == 7);
    ec = purge_ecurve(rev);
    uproc_assert(ec->suffix_count == 4);
}

int main(void)
{
    uproc_error_set_handler(errhandler_bail, NULL);
    memcpy(alphabet_str_, "AGSTPKRQEDNHYWFMLIVC", sizeof alphabet_str_);
    alpha_ = uproc_alphabet_create(alphabet_str_);

    test_mark_singletons();
    test_purge();
    return 0;
}

#endif

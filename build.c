#include "upro.h"
#include <ctype.h>

unsigned long filtered_counts[UPRO_FAMILY_MAX] = { 0 };

struct ecurve_entry
{
    struct upro_word word;
    upro_family family;
};

static int
parse_class(const char *id, upro_family *family)
{
    int res;
    while (*id && !isdigit(*id)) {
        id++;
    }

    res = sscanf(id, "%" UPRO_FAMILY_SCN, family);
    return res == 1 ? UPRO_SUCCESS : UPRO_FAILURE;
}

static int
extract_uniques(upro_io_stream *stream, upro_amino first,
        const struct upro_alphabet *alpha, struct ecurve_entry **entries,
        size_t *n_entries)
{
    int res;

    struct upro_fasta_reader rd;

    size_t index;

    struct upro_bst tree;
    union upro_bst_key tree_key;

    upro_family family;

    upro_fasta_reader_init(&rd, 8192);
    upro_bst_init(&tree, UPRO_BST_WORD, sizeof family);

    while ((res = upro_fasta_read(stream, &rd)) == UPRO_ITER_YIELD) {
        struct upro_worditer iter;
        struct upro_word fwd_word = UPRO_WORD_INITIALIZER, rev_word = UPRO_WORD_INITIALIZER;
        upro_family tmp_family;

        res = parse_class(rd.header, &family);
        upro_worditer_init(&iter, rd.seq, alpha);

        while ((res = upro_worditer_next(&iter, &index, &fwd_word, &rev_word)) == UPRO_ITER_YIELD) {
            if (!upro_word_startswith(&fwd_word, first)) {
                continue;
            }
            tree_key.word = fwd_word;
            res = upro_bst_get(&tree, tree_key, &tmp_family);

            /* word was already present -> mark as duplicate if stored class
             * differs */
            if (res == UPRO_SUCCESS) {
                if (tmp_family != family) {
                    filtered_counts[family] += 1;
                    if (tmp_family != (upro_family)-1) {
                        filtered_counts[tmp_family] += 1;
                    }
                    tmp_family = -1;
                    res = upro_bst_update(&tree, tree_key, &tmp_family);
                }
            }
            else if (res == UPRO_ENOENT) {
                res = upro_bst_insert(&tree, tree_key, &family);
            }

            if (UPRO_ISERROR(res)) {
                break;
            }
        }
        if (UPRO_ISERROR(res)) {
            break;
        }
    }
    if (UPRO_ISERROR(res)) {
        goto error;
    }

    struct upro_bstiter iter;
    upro_bstiter_init(&iter, &tree);
    *n_entries = 0;
    while (upro_bstiter_next(&iter, &tree_key, &family) == UPRO_ITER_YIELD) {
        if (family != (upro_family) -1) {
            *n_entries += 1;
        }
    }
    *entries = malloc(*n_entries * sizeof **entries);
    if (!*entries) {
        res = UPRO_ENOMEM;
        goto error;
    }

    upro_bstiter_init(&iter, &tree);
    struct ecurve_entry *entries_insert = *entries;
    while (upro_bstiter_next(&iter, &tree_key, &family) == UPRO_ITER_YIELD) {
        if (family != (upro_family) -1) {
            entries_insert->word = tree_key.word;
            entries_insert->family = family;
            entries_insert++;
        }
    }

error:
    upro_bst_clear(&tree, NULL);
    return res;
}


static size_t
filter_singletons(struct ecurve_entry *entries, size_t n)
{
    size_t i, k;
    unsigned char *types = calloc(n, sizeof *types);
    enum
    {
        SINGLE,
        CLUSTER,
        BRIDGED,
        CROSSOVER
    };

    for (i = 0; i < n; i++) {
        struct ecurve_entry *e = &entries[i];
        unsigned char *t = &types[i];

        /* |AA..| */
        if (i < n - 1 && e[0].family == e[1].family) {
            t[0] = t[1] = CLUSTER;
        }
        /* |ABA.| */
        else if (i < n - 2 && e[0].family == e[2].family) {
            /* B|ABA.| */
            if (t[1] == BRIDGED || t[1] == CROSSOVER) {
                t[0] = t[1] = t[2] = CROSSOVER;
            }
            /* |ABAB| */
            else if (i < n - 3 && t[0] != CLUSTER && e[1].family == e[3].family) {
                t[0] = t[1] = t[2] = t[3] = CROSSOVER;
            }
            /* A|ABA.| or .|ABA.| */
            else {
                if (t[0] != CLUSTER && t[0] != CROSSOVER) {
                    t[0] = BRIDGED;
                }
                t[2] = BRIDGED;
            }
        }
    }

    for (i = k = 0; i < n; i++) {
        if (types[i] == CLUSTER || types[i] == BRIDGED) {
            entries[k] = entries[i];
            k++;
        }
        else {
            filtered_counts[entries[i].family] += 1;
        }
    }
    free(types);
    return k;
}


static int
insert_entries(struct upro_ecurve *ecurve, struct ecurve_entry *entries, size_t n_entries)
{
    size_t i, k;
    upro_prefix p, current_prefix, next_prefix;

    current_prefix = entries[0].word.prefix;

    for (p = 0; p < current_prefix; p++) {
        ecurve->prefixes[p].first = 0;
        ecurve->prefixes[p].count = UPRO_ECURVE_EDGE;
    }

    for (i = 0; i < n_entries;) {
        ecurve->prefixes[current_prefix].first = i;
        for (k = i; k < n_entries && entries[k].word.prefix == current_prefix; k++) {
            ecurve->suffixes[k] = entries[k].word.suffix;
            ecurve->families[k] = entries[k].family;
            ;
        }
        ecurve->prefixes[current_prefix].first = i;
        ecurve->prefixes[current_prefix].count = k - i;

        if (k == n_entries) {
            break;
        }

        next_prefix = entries[k].word.prefix;

        for (p = current_prefix + 1; p < next_prefix; p++) {
            ecurve->prefixes[p].first = k - 1;
            ecurve->prefixes[p].count = UPRO_ECURVE_EDGE;
        }
        current_prefix = next_prefix;
        i = k;
    }

    for (p = current_prefix + 1; p <= UPRO_PREFIX_MAX; p++) {
        ecurve->prefixes[p].first = n_entries - 1;
        ecurve->prefixes[p].count = UPRO_ECURVE_EDGE;
    }

    return UPRO_SUCCESS;
}


static int
build(upro_io_stream *stream, struct upro_ecurve *ecurve, const char *alphabet)
{
    int res;
    struct ecurve_entry *entries = NULL;
    size_t n_entries;
    upro_amino first;
    struct upro_ecurve new;

    for (first = 0; first < UPRO_ALPHABET_SIZE; first++) {
        fflush(stderr);
        n_entries = 0;
        free(entries);
        upro_io_seek(stream, 0, SEEK_SET);

        res = extract_uniques(stream, first, &ecurve->alphabet, &entries, &n_entries);
        if (UPRO_ISERROR(res)) {
            goto error;
        }

        n_entries = filter_singletons(entries, n_entries);

        if (!first) {
            res = upro_ecurve_init(ecurve, alphabet, n_entries);
            if (UPRO_ISERROR(res)) {
                goto error;
            }
            insert_entries(ecurve, entries, n_entries);
        }
        else {
            res = upro_ecurve_init(&new, alphabet, n_entries);
            if (UPRO_ISERROR(res)) {
                goto error;
            }

            insert_entries(&new, entries, n_entries);
            res = upro_ecurve_append(ecurve, &new);
            if (UPRO_ISERROR(res)) {
                goto error;
            }
        }
    }

    if (0) {
error:
        upro_ecurve_destroy(ecurve);
    }
    free(entries);
    return res;
}

int
main(int argc, char **argv)
{
    int res;
    upro_io_stream *stream;

    struct upro_ecurve ecurve;

    enum args {
        ALPHABET = 1,
        INFILE,
        OUTFILE,
        ARGC
    };

    if (argc != ARGC) {
        fprintf(stderr, "usage: %s ALPHABET INFILE OUTFILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    res = upro_ecurve_init(&ecurve, argv[ALPHABET], 0);
    if (UPRO_ISERROR(res)) {
        fprintf(stderr, "invalid alphabet string \"%s\"\n", argv[ALPHABET]);
        return EXIT_FAILURE;
    }

    stream = upro_io_open(argv[INFILE], "r", UPRO_IO_GZIP);
    if (!stream) {
        perror("");
        return EXIT_FAILURE;
    }

    res = build(stream, &ecurve, argv[ALPHABET]);
    if (UPRO_ISERROR(res)) {
        fprintf(stderr, "an error occured\n");
    }
    upro_io_close(stream);
    fprintf(stderr, "storing..\n");
    upro_storage_store(&ecurve, argv[OUTFILE], UPRO_STORAGE_PLAIN, UPRO_IO_GZIP);
    upro_ecurve_destroy(&ecurve);
    fprintf(stderr, "filtered:\n");
    for (size_t i = 0; i < UPRO_FAMILY_MAX; i++) {
        if (filtered_counts[i]) {
            fprintf(stderr, "%5zu: %lu\n", i, filtered_counts[i]);
        }
    }
    return EXIT_SUCCESS;
}

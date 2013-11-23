#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "uproc.h"
#include <ctype.h>

unsigned long filtered_counts[UPROC_FAMILY_MAX] = { 0 };

struct ecurve_entry
{
    struct uproc_word word;
    uproc_family family;
};

static int
parse_family(const char *id, uproc_family *family)
{
    int res;
    while (*id && !isdigit(*id)) {
        id++;
    }

    res = sscanf(id, "%" UPROC_FAMILY_SCN, family);
    return res == 1 ? 0 : -1;
}

static int
extract_uniques(uproc_io_stream *stream, uproc_amino first,
        const struct uproc_alphabet *alpha, struct ecurve_entry **entries,
        size_t *n_entries)
{
    int res;

    struct uproc_fasta_reader rd;

    size_t index;

    struct uproc_bst tree;
    union uproc_bst_key tree_key;

    uproc_family family;

    uproc_fasta_reader_init(&rd, 8192);
    uproc_bst_init(&tree, UPROC_BST_WORD, sizeof family);

    while ((res = uproc_fasta_read(stream, &rd)) > 0) {
        struct uproc_worditer iter;
        struct uproc_word fwd_word = UPROC_WORD_INITIALIZER, rev_word = UPROC_WORD_INITIALIZER;
        uproc_family tmp_family;

        if (parse_family(rd.header, &family)) {
            break;
        }
        uproc_worditer_init(&iter, rd.seq, alpha);

        while ((res = uproc_worditer_next(&iter, &index, &fwd_word, &rev_word)) > 0) {
            if (!uproc_word_startswith(&fwd_word, first)) {
                continue;
            }
            tree_key.word = fwd_word;
            res = uproc_bst_get(&tree, tree_key, &tmp_family);

            /* word was already present -> mark as duplicate if stored class
             * differs */
            if (res == UPROC_SUCCESS) {
                if (tmp_family != family) {
                    filtered_counts[family] += 1;
                    if (tmp_family != (uproc_family)-1) {
                        filtered_counts[tmp_family] += 1;
                    }
                    tmp_family = -1;
                    res = uproc_bst_update(&tree, tree_key, &tmp_family);
                }
            }
            else if (res == UPROC_BST_KEY_NOT_FOUND) {
                res = uproc_bst_update(&tree, tree_key, &family);
            }
            if (res < 0) {
                break;
            }
        }
        if (res) {
            break;
        }
    }
    if (res) {
        goto error;
    }

    struct uproc_bstiter iter;
    uproc_bstiter_init(&iter, &tree);
    *n_entries = 0;
    while (uproc_bstiter_next(&iter, &tree_key, &family) > 0) {
        if (family != (uproc_family) -1) {
            *n_entries += 1;
        }
    }
    *entries = malloc(*n_entries * sizeof **entries);
    if (!*entries) {
        res = uproc_error(UPROC_ENOMEM);
        goto error;
    }

    uproc_bstiter_init(&iter, &tree);
    struct ecurve_entry *entries_insert = *entries;
    while (uproc_bstiter_next(&iter, &tree_key, &family) > 0) {
        if (family != (uproc_family) -1) {
            entries_insert->word = tree_key.word;
            entries_insert->family = family;
            entries_insert++;
        }
    }

error:
    uproc_bst_clear(&tree, NULL);
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
insert_entries(struct uproc_ecurve *ecurve, struct ecurve_entry *entries, size_t n_entries)
{
    size_t i, k;
    uproc_prefix p, current_prefix, next_prefix;

    current_prefix = entries[0].word.prefix;

    for (p = 0; p < current_prefix; p++) {
        ecurve->prefixes[p].first = 0;
        ecurve->prefixes[p].count = UPROC_ECURVE_EDGE;
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
            ecurve->prefixes[p].count = UPROC_ECURVE_EDGE;
        }
        current_prefix = next_prefix;
        i = k;
    }

    for (p = current_prefix + 1; p <= UPROC_PREFIX_MAX; p++) {
        ecurve->prefixes[p].first = n_entries - 1;
        ecurve->prefixes[p].count = UPROC_ECURVE_EDGE;
    }

    return UPROC_SUCCESS;
}


static int
build(uproc_io_stream *stream, struct uproc_ecurve *ecurve, const char *alphabet)
{
    int res;
    struct ecurve_entry *entries = NULL;
    size_t n_entries;
    uproc_amino first;
    struct uproc_ecurve new;

    for (first = 0; first < UPROC_ALPHABET_SIZE; first++) {
        fflush(stderr);
        n_entries = 0;
        free(entries);
        uproc_io_seek(stream, 0, SEEK_SET);

        res = extract_uniques(stream, first, &ecurve->alphabet, &entries, &n_entries);
        if (res) {
            goto error;
        }

        n_entries = filter_singletons(entries, n_entries);

        if (!first) {
            res = uproc_ecurve_init(ecurve, alphabet, n_entries);
            if (res) {
                goto error;
            }
            insert_entries(ecurve, entries, n_entries);
        }
        else {
            res = uproc_ecurve_init(&new, alphabet, n_entries);
            if (res) {
                goto error;
            }

            insert_entries(&new, entries, n_entries);
            res = uproc_ecurve_append(ecurve, &new);
            if (res) {
                goto error;
            }
        }
    }

    if (0) {
error:
        uproc_ecurve_destroy(ecurve);
    }
    free(entries);
    return res;
}

int
main(int argc, char **argv)
{
    int res;
    uproc_io_stream *stream;

    struct uproc_ecurve ecurve;

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

    res = uproc_ecurve_init(&ecurve, argv[ALPHABET], 0);
    if (res) {
        fprintf(stderr, "invalid alphabet string \"%s\"\n", argv[ALPHABET]);
        return EXIT_FAILURE;
    }

    stream = uproc_io_open("r", UPROC_IO_GZIP, argv[INFILE]);
    if (!stream) {
        perror("");
        return EXIT_FAILURE;
    }

    res = build(stream, &ecurve, argv[ALPHABET]);
    if (res) {
        fprintf(stderr, "an error occured\n");
    }
    uproc_io_close(stream);
    fprintf(stderr, "storing..\n");
    uproc_storage_store(&ecurve, UPROC_STORAGE_PLAIN, UPROC_IO_GZIP, argv[OUTFILE]);
    uproc_ecurve_destroy(&ecurve);
    fprintf(stderr, "filtered:\n");
    for (size_t i = 0; i < UPROC_FAMILY_MAX; i++) {
        if (filtered_counts[i]) {
            fprintf(stderr, "%5zu: %lu\n", i, filtered_counts[i]);
        }
    }
    return EXIT_SUCCESS;
}

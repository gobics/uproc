#include "ecurve.h"
#include <ctype.h>

struct ecurve_entry
{
    struct ec_word word;
    ec_class cls;
};

static int
parse_class(const char *id, ec_class *cls)
{
    int res;
    while (*id && !isdigit(*id)) {
        id++;
    }

    res = sscanf(id, "%" EC_CLASS_SCN, cls);
    return res == 1 ? EC_SUCCESS : EC_FAILURE;
}

static int
walk_cb_count_nondups(union ec_bst_key key, union ec_bst_data data,
        void *opaque)
{
    size_t *n = opaque;
    (void) key;
    if (data.uint != (uintmax_t) -1) {
        (*n)++;
    }
    return EC_SUCCESS;
}

static int
walk_cb_insert_nondups(union ec_bst_key key, union ec_bst_data data,
        void *opaque)
{
    struct ecurve_entry **arr = opaque;
    if (data.uint != (uintmax_t) -1) {
        (*arr)->word = key.word;
        (*arr)->cls = data.uint;
        (*arr)++;
    }
    return EC_SUCCESS;
}

static int
extract_uniques(FILE *stream, ec_amino first, const struct ec_alphabet *alpha,
        struct ecurve_entry **entries, size_t *n_entries)
{
    int res;
    char *id = NULL, *seq = NULL;
    size_t id_sz, seq_sz, index;

    struct ec_bst tree;
    union ec_bst_key tree_key;
    union ec_bst_data tree_data;

    ec_class cls;
    struct ecurve_entry *entries_insert;

    struct ec_worditer iter;
    struct ec_word fwd_word, rev_word;

    ec_bst_init(&tree, EC_BST_WORD);

    while ((res = ec_seqio_fasta_read(stream, NULL, &id, &id_sz, NULL, NULL,
                &seq, &seq_sz)) == EC_ITER_YIELD) {
        res = parse_class(id, &cls);
        ec_worditer_init(&iter, seq, alpha);

        while ((res = ec_worditer_next(&iter, &index, &fwd_word, &rev_word)) == EC_ITER_YIELD) {
            if (!ec_word_startswith(&fwd_word, first)) {
                continue;
            }
            tree_key.word = fwd_word;
            res = ec_bst_get(&tree, tree_key, &tree_data);

            /* word was already present -> mark as duplicate if stored class
             * differs */
            if (res == EC_SUCCESS) {
                if (tree_data.uint != cls) {
                    tree_data.uint = -1;
                    res = ec_bst_update(&tree, tree_key, tree_data);
                }
            }
            else if (res == EC_ENOENT) {
                tree_data.uint = cls;
                res = ec_bst_insert(&tree, tree_key, tree_data);
            }

            if (EC_ISERROR(res)) {
                break;
            }
        }
        if (EC_ISERROR(res)) {
            break;
        }
    }
    free(id);
    free(seq);
    if (EC_ISERROR(res)) {
        goto error;
    }

    *n_entries = 0;
    (void) ec_bst_walk(&tree, walk_cb_count_nondups, n_entries);
    *entries = malloc(*n_entries * sizeof **entries);
    if (!entries) {
        res = EC_ENOMEM;
        goto error;
    }
    entries_insert = *entries;
    (void) ec_bst_walk(&tree, walk_cb_insert_nondups, &entries_insert);

error:
    ec_bst_clear(&tree, NULL);
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
        if (i < n - 1 && e[0].cls == e[1].cls) {
            t[0] = t[1] = CLUSTER;
        }
        /* |ABA.| */
        else if (i < n - 2 && e[0].cls == e[2].cls) {
            /* B|ABA.| */
            if (t[1] == BRIDGED || t[1] == CROSSOVER) {
                t[0] = t[1] = t[2] = CROSSOVER;
            }
            /* |ABAB| */
            else if (i < n - 3 && t[0] != CLUSTER && e[1].cls == e[3].cls) {
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
    }
    free(types);
    return k;
}


static int
insert_entries(struct ec_ecurve *ecurve, struct ecurve_entry *entries, size_t n_entries)
{
    size_t i, k;
    ec_prefix p, current_prefix, next_prefix;

    current_prefix = entries[0].word.prefix;

    for (p = 0; p < current_prefix; p++) {
        ecurve->prefixes[p].first = 0;
        ecurve->prefixes[p].count = EC_ECURVE_EDGE;
    }

    for (i = 0; i < n_entries;) {
        ecurve->prefixes[current_prefix].first = i;
        for (k = i; k < n_entries && entries[k].word.prefix == current_prefix; k++) {
            ecurve->suffixes[k] = entries[k].word.suffix;
            ecurve->classes[k] = entries[k].cls;
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
            ecurve->prefixes[p].count = EC_ECURVE_EDGE;
        }
        current_prefix = next_prefix;
        i = k;
    }

    for (p = current_prefix + 1; p <= EC_PREFIX_MAX; p++) {
        ecurve->prefixes[p].first = n_entries - 1;
        ecurve->prefixes[p].count = EC_ECURVE_EDGE;
    }

    return EC_SUCCESS;
}


static int
build(FILE *stream, struct ec_ecurve *ecurve, const char *alphabet)
{
    int res;
    struct ecurve_entry *entries = NULL;
    size_t n_entries;
    ec_amino first;
    struct ec_ecurve new;

    for (first = 0; first < EC_ALPHABET_SIZE; first++) {
        fflush(stderr);
        n_entries = 0;
        free(entries);
        rewind(stream);

        res = extract_uniques(stream, first, &ecurve->alphabet, &entries, &n_entries);
        if (EC_ISERROR(res)) {
            goto error;
        }

        n_entries = filter_singletons(entries, n_entries);

        if (!first) {
            res = ec_ecurve_init(ecurve, alphabet, n_entries);
            if (EC_ISERROR(res)) {
                goto error;
            }
            insert_entries(ecurve, entries, n_entries);
        }
        else {
            res = ec_ecurve_init(&new, alphabet, n_entries);
            if (EC_ISERROR(res)) {
                goto error;
            }

            insert_entries(&new, entries, n_entries);
            res = ec_ecurve_append(ecurve, &new);
            if (EC_ISERROR(res)) {
                goto error;
            }
        }
    }

    if (0) {
error:
        ec_ecurve_destroy(ecurve);
    }
    free(entries);
    return res;
}

int
main(int argc, char **argv)
{
    int res;
    FILE *stream;

    struct ec_ecurve ecurve;

    enum args {
        ALPHABET = 1,
        INFILE,
        OUTFILE,
        ARGC
    };

    if (argc != ARGC) {
        fprintf(stderr, "usage: %s f|r ALPHABET INFILE OUTFILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    res = ec_ecurve_init(&ecurve, argv[ALPHABET], 0);
    if (EC_ISERROR(res)) {
        fprintf(stderr, "invalid alphabet string \"%s\"\n", argv[ALPHABET]);
        return EXIT_FAILURE;
    }

    stream = fopen(argv[INFILE], "r");
    if (!stream) {
        perror("");
        return EXIT_FAILURE;
    }

    res = build(stream, &ecurve, argv[ALPHABET]);
    if (EC_ISERROR(res)) {
        fprintf(stderr, "an error occured\n");
    }
    fprintf(stderr, "storing..\n");
    ec_storage_store_file(&ecurve, argv[OUTFILE], EC_STORAGE_PLAIN);
    ec_ecurve_destroy(&ecurve);
    return EXIT_SUCCESS;
}

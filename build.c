#include "ecurve.h"

enum direction
{
    FORWARD,
    REVERSE,
};

struct ecurve_entry
{
    struct ec_word word;
    ec_class cls;
};

static int
parse_class(const char *id, ec_class *cls)
{
    int res;

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
        enum direction direction, struct ecurve_entry **entries,
        size_t *n_entries)
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
    struct ec_word fwd_word, rev_word, *word;

    ec_bst_init(&tree, EC_BST_WORD);

    word = direction == FORWARD ? &fwd_word : &rev_word;

    while ((res = ec_seqio_fasta_read(stream, NULL, &id, &id_sz, NULL, NULL,
                &seq, &seq_sz)) == EC_ITER_YIELD) {
        res = parse_class(id, &cls);
        ec_worditer_init(&iter, seq, alpha);

        while ((res = ec_worditer_next(&iter, &index, &fwd_word, &rev_word)) == EC_ITER_YIELD) {
            if (!ec_word_startswith(word, first)) {
                continue;
            }
            tree_key.word = *word;
            tree_data.uint = cls;
            res = ec_bst_insert(&tree, tree_key, tree_data);

            /* word was already present -> mark as duplicate */
            if (res == EC_EEXIST) {
                tree_data.uint = -1;
                res = ec_bst_update(&tree, tree_key, tree_data);
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
grow(struct ec_ecurve *e, size_t n)
{
    void *tmp;
    size_t sz = e->suffix_count + n;
    if (!sz) {
        return EC_SUCCESS;
    }

    tmp = realloc(e->suffixes, sz * sizeof *e->suffixes);
    if (!tmp) {
        return EC_ENOMEM;
    }
    e->suffixes = tmp;

    tmp = realloc(e->classes, sz * sizeof *e->classes);
    if (!tmp) {
        return EC_ENOMEM;
    }
    e->classes = tmp;

    e->suffix_count = sz;
    return EC_SUCCESS;
}

static int
append_entries(struct ec_ecurve *ecurve, ec_amino first,
        struct ecurve_entry *entries, size_t n_entries)
{
    ec_prefix p, current_prefix, next_prefix, last_prefix;
    size_t i, prev_last, offs;
    struct ec_word word = EC_WORD_INITIALIZER;

    ec_word_prepend(&word, first);
    current_prefix = word.prefix;

    ec_word_append(&word, 0);
    ec_word_prepend(&word, first + 1);
    last_prefix = word.prefix - 1;

    offs = prev_last = ecurve->suffix_count;
    if (prev_last) {
        prev_last -= 1;
    }

    if (EC_ISERROR(grow(ecurve, n_entries))) {
        return EC_ENOMEM;
    }

    /* set the first prefix because the gap-filling loop below won't catch it if i == 0 */
    ecurve->prefixes[current_prefix].first = prev_last;
    ecurve->prefixes[current_prefix].count = prev_last ? 0 : -1;

    for (i = 0; i < n_entries; i++) {
        next_prefix = entries[i].word.prefix;

        if (next_prefix != current_prefix) {
            /* fill the gap between old and new prefix */
            prev_last = offs + i;
            if (prev_last) {
                prev_last -= 1;
            }
            for (p = current_prefix + 1; p < next_prefix; p++) {
                ecurve->prefixes[p].first = prev_last;
                ecurve->prefixes[p].count = prev_last ? 0 : -1;
            }

            /* set entries for the new prefix */
            ecurve->prefixes[next_prefix].first = offs + i;
            ecurve->prefixes[next_prefix].count = 0;
            current_prefix = next_prefix;
        }
        ecurve->suffixes[offs + i] = entries[i].word.suffix;
        ecurve->classes[offs + i] = entries[i].cls;
        ecurve->prefixes[current_prefix].count += 1;
    }

    /* fill the remaining entries */
    for (p = current_prefix + 1; p <= last_prefix; p++) {
        ecurve->prefixes[p].first = prev_last;
        ecurve->prefixes[p].count = (first == EC_ALPHABET_SIZE - 1) ? -1 : 0;
    }

    return EC_SUCCESS;
}


static int
build(FILE *stream, enum direction direction, struct ec_ecurve *ecurve)
{
    int res;
    struct ecurve_entry *entries = NULL;
    size_t n_entries;
    ec_amino first;

    for (first = 0; first < EC_ALPHABET_SIZE; first++) {
        fflush(stderr);
        n_entries = 0;
        free(entries);
        rewind(stream);

        res = extract_uniques(stream, first, &ecurve->alphabet, direction, &entries, &n_entries);
        if (EC_ISERROR(res)) {
            goto error;
        }

        n_entries = filter_singletons(entries, n_entries);

        res = append_entries(ecurve, first, entries, n_entries);

        if (EC_ISERROR(res)) {
            goto error;
        }
    }
error:
    free(entries);
    return res;
}

int
main(int argc, char **argv)
{
    int res;
    enum direction direction;
    FILE *stream;

    struct ec_ecurve ecurve;

    enum args {
        DIRECTION = 1,
        ALPHABET,
        INFILE,
        OUTFILE,
        ARGC
    };

    if (argc < ARGC) {
        fprintf(stderr, "usage: %s f|r ALPHABET INFILE OUTFILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    switch (argv[DIRECTION][0]) {
        case 'f':
            direction = FORWARD;
            break;

        case 'r':
            direction = REVERSE;
            break;

        default:
            fprintf(stderr, "first argument must be 'f' (forward) or 'r' (reverse)\n");
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


    res = build(stream, direction, &ecurve);
    if (EC_ISERROR(res)) {
        fprintf(stderr, "an error occured\n");
    }
    fprintf(stderr, "storing..\n");
    ec_storage_store_file(&ecurve, argv[OUTFILE], EC_STORAGE_PLAIN);

    return EXIT_SUCCESS;
}

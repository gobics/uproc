/* uproc-makedb
 * Create a new uproc database.
 *
 * Copyright 2013 Peter Meinicke, Robin Martinjak
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

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <uproc.h>
#include "makedb.h"

#define ALPHA_DEFAULT "AGSTPKRQEDNHYWFMLIVC"

unsigned long filtered_counts[UPROC_FAMILY_MAX] = { 0 };

struct ecurve_entry
{
    struct uproc_word word;
    uproc_family family;
};

static char *
crop_first_word(char *s)
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
    }
    else {
        len = strlen(p1);
    }
    memmove(s, p1, len + 1);
    return s;
}

static char *
reverse_string(char *s)
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

static int
extract_uniques(uproc_io_stream *stream, const struct uproc_alphabet *alpha,
                struct uproc_idmap *idmap, uproc_amino first, bool reverse,
                struct ecurve_entry **entries, size_t *n_entries)
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
        struct uproc_word fwd_word = UPROC_WORD_INITIALIZER,
                          rev_word = UPROC_WORD_INITIALIZER;
        uproc_family tmp_family;

        crop_first_word(rd.header);
        family = uproc_idmap_family(idmap, rd.header);
        if (family == UPROC_FAMILY_INVALID) {
            return -1;
        }

        if (reverse) {
            reverse_string(rd.seq);
        }

        uproc_worditer_init(&iter, rd.seq, alpha);

        while (res = uproc_worditer_next(&iter, &index, &fwd_word, &rev_word),
               res > 0) {
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
                    if (tmp_family != UPROC_FAMILY_INVALID) {
                        filtered_counts[tmp_family] += 1;
                    }
                    tmp_family = UPROC_FAMILY_INVALID;
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
        if (family != UPROC_FAMILY_INVALID) {
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
        if (family != UPROC_FAMILY_INVALID) {
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
insert_entries(struct uproc_ecurve *ecurve, struct ecurve_entry *entries,
               size_t n_entries)
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
        for (k = i;
             k < n_entries && entries[k].word.prefix == current_prefix;
             k++)
        {
            ecurve->suffixes[k] = entries[k].word.suffix;
            ecurve->families[k] = entries[k].family;
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
build_ecurve(const char *infile,
             const char *alphabet,
             struct uproc_idmap *idmap,
             bool reverse,
             struct uproc_ecurve *ecurve)
{
    int res;
    uproc_io_stream *stream;
    struct ecurve_entry *entries = NULL;
    size_t n_entries;
    uproc_amino first;
    struct uproc_ecurve new;
    struct uproc_alphabet alpha;

    res = uproc_alphabet_init(&alpha, alphabet);
    if (res) {
        return res;
    }

    for (first = 0; first < UPROC_ALPHABET_SIZE; first++) {
        progress("building ecurve", first * 100 / UPROC_ALPHABET_SIZE);
        n_entries = 0;
        free(entries);
        stream = uproc_io_open("r", UPROC_IO_GZIP, infile);
        if (!stream) {
            res = -1;
            goto error;
        }
        res = extract_uniques(stream, &alpha, idmap, first, reverse,
                              &entries, &n_entries);
        uproc_io_close(stream);
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
    progress("building ecurve", first * 100 / UPROC_ALPHABET_SIZE);

    if (0) {
error:
        fputc('\n', stderr);
        uproc_ecurve_destroy(ecurve);
    }
    free(entries);
    return res;
}

static int
build_and_store(const char *infile, const char *outdir, const char *alphabet,
                struct uproc_idmap *idmap, bool reverse)
{
    int res;
    struct uproc_ecurve ecurve;
    res = uproc_ecurve_init(&ecurve, alphabet, 0);
    if (res) {
        return res;
    }
    res = build_ecurve(infile, alphabet, idmap, reverse, &ecurve);
    if (res) {
        return res;
    }
    res = uproc_storage_store(&ecurve, UPROC_STORAGE_BINARY, UPROC_IO_GZIP,
                              "%s/%s.ecurve", outdir, reverse ? "rev": "fwd");
    uproc_ecurve_destroy(&ecurve);
    return res;
}

int
build_ecurves(const char *infile,
              const char *outdir,
              const char *alphabet,
              struct uproc_idmap *idmap)
{
    int res;
    res = build_and_store(infile, outdir, alphabet, idmap, false);
    if (res) {
        uproc_perror("error building forward ecurve");
        return res;
    }
    res = build_and_store(infile, outdir, alphabet, idmap, true);
    if (res) {
        uproc_perror("error building reverse ecurve");
    }
    return res;
}

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

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <uproc.h>
#include "common.h"
#include "makedb.h"

unsigned long filtered_counts[UPROC_FAMILY_MAX] = {0};

struct ecurve_entry
{
    struct uproc_word word;
    uproc_family family;
};

static char *crop_first_word(char *s)
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

static char *reverse_string(char *s)
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

static int extract_uniques(uproc_io_stream *stream, const uproc_alphabet *alpha,
                           uproc_idmap *idmap, uproc_amino first, bool reverse,
                           struct ecurve_entry **entries, size_t *n_entries)
{
    int res;

    uproc_seqiter *rd;
    struct uproc_sequence seq;
    size_t index;

    uproc_bst *tree;
    union uproc_bst_key tree_key;

    uproc_family family;

    tree = uproc_bst_create(UPROC_BST_WORD, sizeof family);
    if (!tree) {
        return -1;
    }
    rd = uproc_seqiter_create(stream);
    if (!rd) {
        res = -1;
        goto error;
    }

    while (res = uproc_seqiter_next(rd, &seq), !res) {
        uproc_worditer *iter;
        struct uproc_word fwd_word = UPROC_WORD_INITIALIZER,
                          rev_word = UPROC_WORD_INITIALIZER;
        uproc_family tmp_family;

        crop_first_word(seq.header);
        family = uproc_idmap_family(idmap, seq.header);
        if (family == UPROC_FAMILY_INVALID) {
            res = -1;
            break;
        }

        if (reverse) {
            reverse_string(seq.data);
        }

        iter = uproc_worditer_create(seq.data, alpha);
        if (!iter) {
            res = -1;
            break;
        }

        while (res = uproc_worditer_next(iter, &index, &fwd_word, &rev_word),
               !res) {
            if (!uproc_word_startswith(&fwd_word, first)) {
                continue;
            }
            tree_key.word = fwd_word;
            res = uproc_bst_get(tree, tree_key, &tmp_family);

            /* word was already present -> mark as duplicate if stored class
             * differs */
            if (!res) {
                if (tmp_family != family) {
                    filtered_counts[family] += 1;
                    if (tmp_family != UPROC_FAMILY_INVALID) {
                        filtered_counts[tmp_family] += 1;
                    }
                    tmp_family = UPROC_FAMILY_INVALID;
                    res = uproc_bst_update(tree, tree_key, &tmp_family);
                }
            } else if (res == UPROC_BST_KEY_NOT_FOUND) {
                res = uproc_bst_update(tree, tree_key, &family);
            }
            if (res) {
                break;
            }
        }
        uproc_worditer_destroy(iter);
        if (res < 0) {
            break;
        }
    }
    uproc_seqiter_destroy(rd);
    if (res == -1) {
        goto error;
    }

    uproc_bstiter *iter;
    iter = uproc_bstiter_create(tree);
    if (!iter) {
        res = -1;
        goto error;
    }
    *n_entries = 0;
    while (!uproc_bstiter_next(iter, &tree_key, &family)) {
        if (family != UPROC_FAMILY_INVALID) {
            *n_entries += 1;
        }
    }
    *entries = malloc(*n_entries * sizeof **entries);
    if (!*entries) {
        res = uproc_error(UPROC_ENOMEM);
        goto error;
    }
    uproc_bstiter_destroy(iter);

    iter = uproc_bstiter_create(tree);
    if (!iter) {
        res = -1;
        goto error;
    }
    struct ecurve_entry *entries_insert = *entries;
    while (!uproc_bstiter_next(iter, &tree_key, &family)) {
        if (family != UPROC_FAMILY_INVALID) {
            entries_insert->word = tree_key.word;
            entries_insert->family = family;
            entries_insert++;
        }
    }
    uproc_bstiter_destroy(iter);

    res = 0;
error:
    uproc_bst_destroy(tree);
    return res;
}

static size_t filter_singletons(struct ecurve_entry *entries, size_t n)
{
    size_t i, k;
    unsigned char *types = calloc(n, sizeof *types);
    enum { SINGLE, CLUSTER, BRIDGED, CROSSOVER };

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
            else if (i < n - 3 && t[0] != CLUSTER &&
                     e[1].family == e[3].family) {
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
        } else {
            filtered_counts[entries[i].family] += 1;
        }
    }
    free(types);
    return k;
}

static int insert_entries(uproc_ecurve *ecurve, struct ecurve_entry *entries,
                          size_t n_entries)
{
    int res = 0;
    size_t i;
    uproc_prefix current_prefix;
    uproc_list *suffix_list;
    struct uproc_ecurve_suffixentry suffix_entry;

    suffix_list = uproc_list_create(sizeof suffix_entry);
    if (!suffix_list) {
        return -1;
    }

    current_prefix = entries[0].word.prefix;

    for (i = 0; i < n_entries; i++) {
        if (entries[i].word.prefix != current_prefix) {
            res = uproc_ecurve_add_prefix(ecurve, current_prefix, suffix_list);
            if (res) {
                goto error;
            }
            uproc_list_clear(suffix_list);
            current_prefix = entries[i].word.prefix;
        }
        suffix_entry.suffix = entries[i].word.suffix;
        suffix_entry.family = entries[i].family;
        res = uproc_list_append(suffix_list, &suffix_entry);
        if (res) {
            goto error;
        }
    }
    res = uproc_ecurve_add_prefix(ecurve, current_prefix, suffix_list);
error:
    uproc_list_destroy(suffix_list);
    return res;
}

static int build_ecurve(const char *infile, const char *alphabet,
                        uproc_idmap *idmap, bool reverse, uproc_ecurve **ecurve)
{
    int res = 0;
    uproc_io_stream *stream;
    struct ecurve_entry *entries = NULL;
    size_t n_entries;
    uproc_amino first = 0;
    uproc_alphabet *alpha;

    alpha = uproc_alphabet_create(alphabet);
    if (!alpha) {
        return -1;
    }

    *ecurve = uproc_ecurve_create(alphabet, 0);
    if (!*ecurve) {
        goto error;
    }

    progress(uproc_stderr, reverse ? "rev.ecurve" : "fwd.ecurve", -1.0);
    for (; first < UPROC_ALPHABET_SIZE; first++) {
        n_entries = 0;
        free(entries);
        stream = uproc_io_open("r", UPROC_IO_GZIP, infile);
        if (!stream) {
            res = -1;
            goto error;
        }
        progress(uproc_stderr, NULL, first * 100 / UPROC_ALPHABET_SIZE);
        res = extract_uniques(stream, alpha, idmap, first, reverse, &entries,
                              &n_entries);
        uproc_io_close(stream);
        if (res) {
            goto error;
        }

        n_entries = filter_singletons(entries, n_entries);
        if (!n_entries) {
            continue;
        }

        res = insert_entries(*ecurve, entries, n_entries);
        if (res) {
            goto error;
        }
    }
    uproc_ecurve_finalize(*ecurve);
    progress(uproc_stderr, NULL, first * 100 / UPROC_ALPHABET_SIZE);

    if (0) {
    error:
        if (first) {
            fputc('\n', stderr);
            uproc_ecurve_destroy(*ecurve);
            *ecurve = NULL;
        }
    }
    uproc_alphabet_destroy(alpha);
    free(entries);
    return res;
}

static int build_and_store(const char *infile, const char *outdir,
                           const char *alphabet, uproc_idmap *idmap,
                           bool reverse)
{
    int res;
    uproc_ecurve *ecurve = NULL;
    res = build_ecurve(infile, alphabet, idmap, reverse, &ecurve);
    if (res) {
        return res;
    }
    fprintf(stderr, "Storing %s/%s.ecurve...", outdir, reverse ? "rev" : "fwd");
    res = uproc_ecurve_store(ecurve, UPROC_ECURVE_BINARY, UPROC_IO_GZIP,
                             "%s/%s.ecurve", outdir, reverse ? "rev" : "fwd");
    uproc_ecurve_destroy(ecurve);
    fprintf(stderr, " Done.");
    return res;
}

int build_ecurves(const char *infile, const char *outdir, const char *alphabet,
                  uproc_idmap *idmap)
{
    int res;
    res = build_and_store(infile, outdir, alphabet, idmap, false);
    if (res) {
        return res;
    }
    res = build_and_store(infile, outdir, alphabet, idmap, true);
    return res;
}

/* Manipulate amino acid words
 *
 * Copyright 2013 Peter Meinicke, Robin Martinjak
 *
 * This file is part of libuproc.
 *
 * libuproc is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * libuproc is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libuproc.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdint.h>

#include "uproc/common.h"
#include "uproc/word.h"
#include "uproc/error.h"

#define AMINO_AT(x, n) (((x) >> (UPROC_AMINO_BITS * (n))) & UPROC_BITMASK(UPROC_AMINO_BITS))

int
uproc_word_from_string(struct uproc_word *word, const char *str,
                    const struct uproc_alphabet *alpha)
{
    int i;
    uproc_amino a;
    for (i = 0; str[i] && i < UPROC_WORD_LEN; i++) {
        a = uproc_alphabet_char_to_amino(alpha, str[i]);
        if (a < 0) {
            return uproc_error_msg(UPROC_EINVAL, "invalid amino acid '%c'",
                                  str[i]);
        }
        uproc_word_append(word, a);
    }
    if (i < UPROC_WORD_LEN) {
        return uproc_error_msg(UPROC_EINVAL,
                              "input string too short: %d chars instead of %d",
                              i, UPROC_WORD_LEN);
    }
    return 0;
}

int
uproc_word_to_string(char *str, const struct uproc_word *word,
                  const struct uproc_alphabet *alpha)
{
    int i, c;
    uproc_prefix p = word->prefix;
    uproc_suffix s = word->suffix;

    i = UPROC_PREFIX_LEN;
    while (i--) {
        c = uproc_alphabet_amino_to_char(alpha, p % UPROC_ALPHABET_SIZE);
        if (c < 0) {
            return uproc_error_msg(UPROC_EINVAL, "invalid word");
        }
        str[i] = c;
        p /= UPROC_ALPHABET_SIZE;
    }

    i = UPROC_SUFFIX_LEN;
    while (i--) {
        c = uproc_alphabet_amino_to_char(alpha, AMINO_AT(s, 0));
        if (c < 0) {
            return uproc_error_msg(UPROC_EINVAL, "invalid word");
        }
        str[i + UPROC_PREFIX_LEN] = c;
        s >>= UPROC_AMINO_BITS;
    }
    str[UPROC_WORD_LEN] = '\0';
    return 0;
}

void
uproc_word_append(struct uproc_word *word, uproc_amino amino)
{
    /* leftmost AA of suffix */
    uproc_amino a = AMINO_AT(word->suffix, UPROC_SUFFIX_LEN - 1);

    /* "shift" left */
    word->prefix *= UPROC_ALPHABET_SIZE;
    word->prefix %= UPROC_PREFIX_MAX + 1;
    word->suffix <<= UPROC_AMINO_BITS;
    word->suffix &= UPROC_BITMASK(UPROC_SUFFIX_LEN * UPROC_AMINO_BITS);

    /* append AA */
    word->prefix += a;
    word->suffix |= amino;
}

void
uproc_word_prepend(struct uproc_word *word, uproc_amino amino)
{
    /* rightmost AA of prefix */
    uproc_amino a = (word->prefix % UPROC_ALPHABET_SIZE);

    /* "shift" right" */
    word->prefix /= UPROC_ALPHABET_SIZE;
    word->suffix >>= UPROC_AMINO_BITS;

    /* prepend AA */
    word->prefix += amino * ((UPROC_PREFIX_MAX + 1) / UPROC_ALPHABET_SIZE);
    word->suffix |= (uproc_suffix)a << (UPROC_AMINO_BITS * (UPROC_SUFFIX_LEN - 1));
}

bool
uproc_word_startswith(const struct uproc_word *word, uproc_amino amino)
{
    uproc_amino first = word->prefix / ((UPROC_PREFIX_MAX + 1) / (UPROC_ALPHABET_SIZE));
    return first == amino;
}

bool
uproc_word_equal(const struct uproc_word *w1, const struct uproc_word *w2)
{
    return w1->prefix == w2->prefix && w1->suffix == w2->suffix;
}

int
uproc_word_cmp(const struct uproc_word *w1, const struct uproc_word *w2)
{
    if (w1->prefix == w2->prefix) {
        if (w1->suffix == w2->suffix) {
            return 0;
        }
        if (w1->suffix < w2->suffix) {
            return -1;
        }
        return 1;
    }
    if (w1->prefix < w2->prefix) {
        return -1;
    }
    return 1;
}

void
uproc_worditer_init(struct uproc_worditer *iter, const char *seq,
                 const struct uproc_alphabet *alpha)
{
    iter->sequence = seq;
    iter->index = 0;
    iter->alphabet = alpha;
}

int
uproc_worditer_next(struct uproc_worditer *iter, size_t *index,
                 struct uproc_word *fwd_word, struct uproc_word *rev_word)
{
    int c;
    uproc_amino a;
    size_t n = (iter->index) ? (UPROC_WORD_LEN - 1) : 0;

    while (n < UPROC_WORD_LEN) {
        if (!(c = iter->sequence[iter->index++])) {
            /* end of sequence reached -> stop iteration */
            return 0;
        }
        if ((a = uproc_alphabet_char_to_amino(iter->alphabet, c)) == -1) {
            /* invalid character -> begin new word */
            n = 0;
            continue;
        }
        n++;
        uproc_word_append(fwd_word, a);
        uproc_word_prepend(rev_word, a);
    }
    *index = iter->index - UPROC_WORD_LEN;
    return 1;
}

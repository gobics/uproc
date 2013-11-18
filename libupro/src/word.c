#include <stdlib.h>
#include <stdint.h>

#include "upro/common.h"
#include "upro/word.h"
#include "upro/error.h"

#define AMINO_AT(x, n) (((x) >> (UPRO_AMINO_BITS * (n))) & UPRO_BITMASK(UPRO_AMINO_BITS))

int
upro_word_from_string(struct upro_word *word, const char *str,
                    const struct upro_alphabet *alpha)
{
    int i;
    upro_amino a;
    for (i = 0; str[i] && i < UPRO_WORD_LEN; i++) {
        a = upro_alphabet_char_to_amino(alpha, str[i]);
        if (a < 0) {
            return upro_error_msg(UPRO_EINVAL, "invalid amino acid '%c'",
                                  str[i]);
        }
        upro_word_append(word, a);
    }
    if (i < UPRO_WORD_LEN) {
        return upro_error_msg(UPRO_EINVAL,
                              "input string too short: %d chars instead of %d",
                              i, UPRO_WORD_LEN);
    }
    return UPRO_SUCCESS;
}

int
upro_word_to_string(char *str, const struct upro_word *word,
                  const struct upro_alphabet *alpha)
{
    int i, c;
    upro_prefix p = word->prefix;
    upro_suffix s = word->suffix;

    i = UPRO_PREFIX_LEN;
    while (i--) {
        c = upro_alphabet_amino_to_char(alpha, p % UPRO_ALPHABET_SIZE);
        if (c < 0) {
            return upro_error_msg(UPRO_EINVAL, "invalid word");
        }
        str[i] = c;
        p /= UPRO_ALPHABET_SIZE;
    }

    i = UPRO_SUFFIX_LEN;
    while (i--) {
        c = upro_alphabet_amino_to_char(alpha, AMINO_AT(s, 0));
        if (c < 0) {
            return upro_error_msg(UPRO_EINVAL, "invalid word");
        }
        str[i + UPRO_PREFIX_LEN] = c;
        s >>= UPRO_AMINO_BITS;
    }
    str[UPRO_WORD_LEN] = '\0';
    return UPRO_SUCCESS;
}

void
upro_word_append(struct upro_word *word, upro_amino amino)
{
    /* leftmost AA of suffix */
    upro_amino a = AMINO_AT(word->suffix, UPRO_SUFFIX_LEN - 1);

    /* "shift" left */
    word->prefix *= UPRO_ALPHABET_SIZE;
    word->prefix %= UPRO_PREFIX_MAX + 1;
    word->suffix <<= UPRO_AMINO_BITS;
    word->suffix &= UPRO_BITMASK(UPRO_SUFFIX_LEN * UPRO_AMINO_BITS);

    /* append AA */
    word->prefix += a;
    word->suffix |= amino;
}

void
upro_word_prepend(struct upro_word *word, upro_amino amino)
{
    /* rightmost AA of prefix */
    upro_amino a = (word->prefix % UPRO_ALPHABET_SIZE);

    /* "shift" right" */
    word->prefix /= UPRO_ALPHABET_SIZE;
    word->suffix >>= UPRO_AMINO_BITS;

    /* prepend AA */
    word->prefix += amino * ((UPRO_PREFIX_MAX + 1) / UPRO_ALPHABET_SIZE);
    word->suffix |= (upro_suffix)a << (UPRO_AMINO_BITS * (UPRO_SUFFIX_LEN - 1));
}

bool
upro_word_startswith(const struct upro_word *word, upro_amino amino)
{
    upro_amino first = word->prefix / ((UPRO_PREFIX_MAX + 1) / (UPRO_ALPHABET_SIZE));
    return first == amino;
}

bool
upro_word_equal(const struct upro_word *w1, const struct upro_word *w2)
{
    return w1->prefix == w2->prefix && w1->suffix == w2->suffix;
}

int
upro_word_cmp(const struct upro_word *w1, const struct upro_word *w2)
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
upro_worditer_init(struct upro_worditer *iter, const char *seq,
                 const struct upro_alphabet *alpha)
{
    iter->sequence = seq;
    iter->index = 0;
    iter->alphabet = alpha;
}

int
upro_worditer_next(struct upro_worditer *iter, size_t *index,
                 struct upro_word *fwd_word, struct upro_word *rev_word)
{
    int c;
    upro_amino a;
    size_t n = (iter->index) ? (UPRO_WORD_LEN - 1) : 0;

    while (n < UPRO_WORD_LEN) {
        if (!(c = iter->sequence[iter->index++])) {
            /* end of sequence reached -> stop iteration */
            return UPRO_ITER_STOP;
        }
        if ((a = upro_alphabet_char_to_amino(iter->alphabet, c)) == -1) {
            /* invalid character -> begin new word */
            n = 0;
            continue;
        }
        n++;
        upro_word_append(fwd_word, a);
        upro_word_prepend(rev_word, a);
    }
    *index = iter->index - UPRO_WORD_LEN;
    return UPRO_ITER_YIELD;
}

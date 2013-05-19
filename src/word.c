#include <stdlib.h>
#include <stdint.h>

#include "ecurve/common.h"
#include "ecurve/word.h"

#define AMINO_MASK (~(~0 << EC_AMINO_BITS))
#define AMINO_AT(x, n) (((x) >> (EC_AMINO_BITS * (n))) & AMINO_MASK)

int
ec_word_from_string(struct ec_word *word, const char *str, const ec_alphabet *alpha)
{
    int i;
    ec_amino a;
    for (i = 0; str[i] && i < EC_WORD_LEN; i++) {
        a = ec_alphabet_char_to_amino(alpha, str[i]);
        if (a < 0) {
            return EC_FAILURE;
        }
        ec_word_append(word, a);
    }
    return (i < EC_WORD_LEN) ? EC_FAILURE : EC_SUCCESS;
}

int
ec_word_to_string(char *str, const struct ec_word *word, const ec_alphabet *alpha)
{
    int i, c;
    ec_prefix p = word->prefix;
    ec_suffix s = word->suffix;

    i = EC_PREFIX_LEN;
    while (i--) {
        c = ec_alphabet_amino_to_char(alpha, p % EC_ALPHABET_SIZE);
        if (c < 0) {
            return EC_FAILURE;
        }
        str[i] = c;
        p /= EC_ALPHABET_SIZE;
    }

    i = EC_SUFFIX_LEN;
    while (i--) {
        c = ec_alphabet_amino_to_char(alpha, AMINO_AT(s, 0));
        if (c < 0) {
            return EC_FAILURE;
        }
        str[i + EC_PREFIX_LEN] = c;
        s >>= EC_AMINO_BITS;
    }
    str[EC_WORD_LEN] = '\0';
    return EC_SUCCESS;
}

void
ec_word_append(struct ec_word *word, ec_amino amino)
{
    /* leftmost AA of suffix */
    ec_amino a = AMINO_AT(word->suffix, EC_SUFFIX_LEN - 1);

    /* "shift" left */
    word->prefix = (word->prefix * EC_ALPHABET_SIZE) % (EC_PREFIX_MAX + 1);
    word->suffix = EC_SUFFIX_MASK(word->suffix << EC_AMINO_BITS);

    /* append AA */
    word->prefix += a;
    word->suffix |= amino;
}

void
ec_word_prepend(struct ec_word *word, ec_amino amino)
{
    /* rightmost AA of prefix */
    ec_amino a = (word->prefix % EC_ALPHABET_SIZE);

    /* "shift" right" */
    word->prefix /= EC_ALPHABET_SIZE;
    word->suffix >>= EC_AMINO_BITS;

    /* prepend AA */
    word->prefix += amino * ((EC_PREFIX_MAX + 1) / EC_ALPHABET_SIZE);
    word->suffix |= (ec_suffix)a << (EC_AMINO_BITS * (EC_SUFFIX_LEN - 1));
}

int
ec_word_cmp(const struct ec_word *w1, const struct ec_word *w2)
{
    if (w1->prefix == w2->prefix) {
        if (w1->suffix == w2->suffix) {
            return 0;
        }
        return w1->suffix > w2->suffix ? 1 : -1;
    }
    return w1->prefix > w2->prefix ? 1 : -1;
}

void
ec_worditer_init(ec_worditer *iter, const char *seq,
                 const ec_alphabet *alpha)
{
    iter->sequence = seq;
    iter->index = 0;
    iter->alphabet = alpha;
}

int
ec_worditer_next(ec_worditer *iter, size_t *index,
                 struct ec_word *fwd_word, struct ec_word *rev_word)
{
    int c;
    ec_amino a;
    size_t n = (iter->index) ? (EC_WORD_LEN - 1) : 0;

    while (n < EC_WORD_LEN) {
        if (!(c = iter->sequence[iter->index++])) {
            /* end of sequence reached -> stop iteration */
            return EC_FAILURE;
        }
        if ((a = ec_alphabet_char_to_amino(iter->alphabet, c)) == -1) {
            /* invalid character -> begin new word */
            n = 0;
            continue;
        }
        n++;
        ec_word_append(fwd_word, a);
        ec_word_prepend(rev_word, a);
    }
    *index = iter->index - EC_WORD_LEN;
    return EC_SUCCESS;
}
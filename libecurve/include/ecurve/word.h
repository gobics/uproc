#ifndef EC_WORD_H
#define EC_WORD_H

/** \file ecurve/word.h
 *
 * Manipulate amino were acid words
 */

#include <stdint.h>
#include <stdbool.h>
#include "ecurve/alphabet.h"

/** Struct defining an amino acid word */
struct ec_word {
    /** First few amino acids */
    ec_prefix prefix;

    /** Last few amino acids */
    ec_suffix suffix;
};

/** Initializer to be used for all `struct ec_word` objects */
#define EC_WORD_INITIALIZER { 0, 0 }

/** Transform a string to amino acid word
 *
 * Translates the first `#EC_WORD_LEN` characters of the given string to a word
 * object. Failure occurs if the string ends or an invalid character is
 * encountered.
 *
 * \param word  _OUT_: amino acid word
 * \param str   string representation
 * \param alpha alphabet to use for translation
 *
 * \retval #EC_SUCCESS  the string was translated successfully
 * \retval #EC_FAILURE  the string was too short or contained an invalid character
 */
int ec_word_from_string(struct ec_word *word, const char *str,
                        const struct ec_alphabet *alpha);

/** Build string corresponding to amino acid word
 *
 * Translates an amino acid word back to a string. The string will be
 * null-terminated, thus `str` should point to a buffer of at least
 * `#EC_WORD_LEN + 1` bytes.
 *
 * \param str   buffer to store the string in
 * \param word  amino acid word
 * \param alpha alphabet to use for translation
 *
 * \retval #EC_SUCCESS  the word was translated successfully
 * \retval #EC_FAILURE  the word contained an invalid amino acid
 */
int ec_word_to_string(char *str, const struct ec_word *word,
                      const struct ec_alphabet *alpha);

/** Append amino acid
 *
 * The leftmost amino acid of `word` is removed, for example
 *   `append(ANERD, S) == NERDS`
 * (in case of a word length of 5).
 *
 * NOTE: It will _not_ be verified whether `amino` is a valid amino acid
 *
 * \param word  word to append to
 * \param amino amino acid to append
 */
void ec_word_append(struct ec_word *word, ec_amino amino);

/** Prepend amino acid
 *
 * Complementary to the append operation, e.g.
 *   `prepend(NERDS, A) == ANERD`.
 *
 * \param word  word to prepend to
 * \param amino amino acid to prepend
 */
void ec_word_prepend(struct ec_word *word, ec_amino amino);

/** Compare first amino acid of a word */
bool ec_word_startswith(const struct ec_word *word, ec_amino amino);

/** Test for equality
 *
 * \return `true` if the words are equal or `false` if not.
 */
bool ec_word_equal(const struct ec_word *word1, const struct ec_word *word2);

/** Iterator over all words in an amino acid sequence */
struct ec_worditer {
    /** Iterated sequence */
    const char *sequence;

    /** Index of the next character to read */
    size_t index;

    /** Translation alphabet */
    const struct ec_alphabet *alphabet;
};

/** Initialize an iterator over a sequence
 *
 * The iterator may not be used after the lifetime of the objects pointed to
 * by `seq` and `alpha` has ended (only the pointer value is stored).
 *
 * \param iter  _OUT_: initialized iterator
 * \param seq   sequence to iterate
 * \param alpha translation alphabet
 */
void ec_worditer_init(struct ec_worditer *iter, const char *seq,
                      const struct ec_alphabet *alpha);

/** Obtain the next word(s) from a word iterator
 *
 * Invalid characters are not simply skipped, instead the first complete after
 * such character is returned next.
 *
 * \param iter      iterator
 * \param index     _OUT_: starting index of the current "forward" word
 * \param fwd_word  _OUT_: word to append the next character to
 * \param rev_word  _OUT_: word to prepend the next character to
 *
 * \retval #EC_SUCCESS      a pair of words was read
 * \retval #EC_ITER_STOP    the iterator is exhausted (i.e. the end of the
 *                          sequence was reached, no words were read)
 */
int ec_worditer_next(struct ec_worditer *iter, size_t *index,
                     struct ec_word *fwd_word, struct ec_word *rev_word);

#endif

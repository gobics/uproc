/* Copyright 2014 Peter Meinicke, Robin Martinjak
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

/** \file uproc/word.h
 *
 * Module: \ref grp_intern_word
 *
 * \weakgroup grp_intern
 * \{
 *
 * \weakgroup grp_intern_word
 * \{
 */

#ifndef UPROC_WORD_H
#define UPROC_WORD_H

#include <stdint.h>
#include <stdbool.h>
#include "uproc/alphabet.h"

/** \defgroup struct_word struct uproc_word
 *
 * Amino acid word
 *
 * \{
 */

/** \copybrief struct_word */
struct uproc_word {
    /** First few amino acids */
    uproc_prefix prefix;

    /** Last few amino acids */
    uproc_suffix suffix;
};

/** Initializer to be used for all ::uproc_word structs */
#define UPROC_WORD_INITIALIZER \
    {                          \
        0, 0                   \
    }

/** Transform a string to amino acid word
 *
 * Translates the first ::UPROC_WORD_LEN characters of the given string to a
 * word object. Failure occurs if the string ends or an invalid character is
 * encountered.
 *
 * \param word      _OUT_: amino acid word
 * \param str       string representation
 * \param alpha     alphabet to use for translation
 */
int uproc_word_from_string(struct uproc_word *word, const char *str,
                           const uproc_alphabet *alpha);

/** Build string corresponding to amino acid word
 *
 * Translates an amino acid word back to a string. The string will be
 * null-terminated, thus \c str should point to a buffer of at least
 * <tt>::UPROC_WORD_LEN + 1</tt> bytes.
 *
 * \param str       buffer to store the string in
 * \param word      amino acid word
 * \param alpha     alphabet to use for translation
 */
int uproc_word_to_string(char *str, const struct uproc_word *word,
                         const uproc_alphabet *alpha);

/** Append amino acid
 *
 * The leftmost amino acid of \c word is removed, for example (in case of a
 * word length of 5):
 *   \code append(ANERD, S) == NERDS \endcode
 *
 * <em> \b NOTE: It will not be verified whether \c amino represents a valid
 * amino acid.</em>
 *
 * \param word      word to append to
 * \param amino     amino acid to append
 */
void uproc_word_append(struct uproc_word *word, uproc_amino amino);

/** Prepend amino acid
 *
 * Complement to the append operation, e.g.
 *   \code prepend(NERDS, A) == ANERD \endcode
 *
 * \param word      word to prepend to
 * \param amino     amino acid to prepend
 */
void uproc_word_prepend(struct uproc_word *word, uproc_amino amino);

/** Compare first amino acid of a word */
bool uproc_word_startswith(const struct uproc_word *word, uproc_amino amino);

/** Compare words
 *
 * Comparison is done lexcographically (prefix first, suffix second).
 *
 * \return  -1, 0 or 1 if \c w1 is less than, equal to, or greater than \c w2.
 */
int uproc_word_cmp(const struct uproc_word *w1, const struct uproc_word *w2);
/** \} */

/** \defgroup obj_worditer object uproc_worditer
 * \{
 */

/** Iterator over all words in an amino acid sequence
 *
 * Produces all consecutive amino acid words of length ::UPROC_WORD_LEN found
 * in a sequence. If a character is encountered that is not an amino acid, the
 * next word will start after that character.
 *
 * Example:
 *
 * For a word length of 5, the sequence
 *
 * \code
 * ANERDSP!FQQRAR
 * \endcode
 *
 * contains the (forward) words:
 * \code
 * ADERD
 * NERDS
 * ERDSP
 * FQQRA
 * QQRAR
 * \endcode
 */
typedef struct uproc_worditer_s uproc_worditer;

/** Create worditer object
 *
 * The iterator may not be used after the lifetime of the objects pointed to
 * by \c seq and \c alpha has ended (only the pointer value is stored).
 *
 * \param seq       sequence to iterate
 * \param alpha     translation alphabet
 */
uproc_worditer *uproc_worditer_create(const char *seq,
                                      const uproc_alphabet *alpha);

/** Obtain the next word(s) from a word iterator
 *
 * Invalid characters are not simply skipped, instead the first complete word
 * after such character is returned next.
 *
 * \param iter      iterator
 * \param index     _OUT_: starting index of the current "forward" word
 * \param fwd       _OUT_: read word in order as it appeared
 * \param rev       _OUT_: read word in reversed order
 */
int uproc_worditer_next(uproc_worditer *iter, size_t *index,
                        struct uproc_word *fwd, struct uproc_word *rev);

/** Destroy worditer object */
void uproc_worditer_destroy(uproc_worditer *iter);
/** \} */

/**
 * \}
 * \}
 */
#endif

#ifndef UPROC_ALPHABET_H
#define UPROC_ALPHABET_H

/** \file uproc/alphabet.h
 *
 * Translate characters to/from amino acids
 */

#include "uproc/common.h"
#include <limits.h>

/** Struct defining an amino acid alphabet */
struct uproc_alphabet
{
    /** Original alphabet string */
    char str[UPROC_ALPHABET_SIZE + 1];

    /** Lookup table for mapping characters to amino acids */
    uproc_amino aminos[UCHAR_MAX + 1];
};

/** Initialize an alphabet object with the given string
 *
 * The string must be exactly ::UPROC_ALPHABET_SIZE characters long and consist
 * only of uppercase letters. No letter shall be included twice.
 *
 * \return
 * On success, returns 0.
 * \return
 * On failure, returns a negative value, sets ::uproc_errno to ::UPROC_EINVAL and
 * ::uproc_errmsg accordingly.
 */
int uproc_alphabet_init(struct uproc_alphabet *alpha, const char *s);

/** Translate character to amino acid
 *
 * \param alpha     alphabet to use
 * \param c         character to translate
 *
 * \return
 * The value (between 0 and ::UPROC_ALPHABET_SIZE) of the corresponding amino
 * acid or -1 if \c c is a non-alphabetic character.
 *
 * Does not set ::uproc_errno.
 */
uproc_amino uproc_alphabet_char_to_amino(const struct uproc_alphabet *alpha,
                                       int c);

/** Translate amino acid to character
 *
 * \param alpha     alphabet to use
 * \param a         amino acid to translate
 *
 * \return
 * Corresponding character, or -1 if `a` does not represent a valid amino acid.
 *
 * Does not set ::uproc_errno.
 */
int uproc_alphabet_amino_to_char(const struct uproc_alphabet *alpha,
                                uproc_amino a);

#endif

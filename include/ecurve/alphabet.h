#ifndef EC_ALPHABET_H
#define EC_ALPHABET_H

/** \file ecurve/alphabet.h
 *
 * Translate characters to/from amino acids
 */

#include "ecurve/common.h"
#include <limits.h>

/** Type to represent an amino acid alphabet. */
typedef struct ec_alphabet_s ec_alphabet;

/** Struct defining an amino acid alphabet
 *
 * Applications should use the #ec_alphabet typedef instead.
 */
struct ec_alphabet_s {
    /** Original alphabet string */
    char str[EC_ALPHABET_SIZE + 1];

    /** Lookup table for mapping characters to amino acids */
    ec_amino amino_table[UCHAR_MAX + 1];
};

#define EC_ALPHABET_ALPHA_DEFAULT EC_ALPHABET_ALPHA_BLOS
#define EC_ALPHABET_ALPHA_BLOS      "AGSTPKRQEDNHYWFMLIVC"
#define EC_ALPHABET_ALPHA_PFAM      "WFLMIVCAPTSGNDEQKRHY"
#define EC_ALPHABET_ALPHA_SAMMON    "WCFYLIVMTASQKRENDGHP"

/** Initialize an alphabet object with the given string */
int ec_alphabet_init(ec_alphabet *alpha, const char *s);

/** Translate character to amino acid
 *
 * \param alpha     alphabet to use
 * \param c         character to translate
 *
 * \return
 * Corresponding amino acid or -1 if `c` is a non-alphabetic character.
 */
ec_amino ec_alphabet_char_to_amino(const ec_alphabet *alpha, int c);

/** Translate amino acid to character
 *
 * \param alpha     alphabet to use
 * \param a         amino acid to translate
 *
 * \return
 * Corresponding character, or -1 if `a` does not represent a valid amino acid.
 */
int ec_alphabet_amino_to_char(const ec_alphabet *alpha, ec_amino a);

#endif

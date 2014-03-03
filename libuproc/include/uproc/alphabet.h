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

/** \file uproc/alphabet.h
 *
 * Convert between the character and internal representation of amino acids.
 *
 * \weakgroup grp_intern
 * @{
 *
 * \weakgroup grp_intern_alpha
 * @{
 */

#ifndef UPROC_ALPHABET_H
#define UPROC_ALPHABET_H

#include "uproc/common.h"


/** Alphabet type
 *
 * An object of this type is required to translate between characters and the
 * internal representation of amino acids. The order of this alphabet is very
 * significant to the results. Each ::uproc_ecurve has an "intrinsic" alphabet
 * and should not be used with a ::uproc_substmat that was derived from a
 * different alphabet.
 */
typedef struct uproc_alphabet_s uproc_alphabet;


/** Create alphabet object
 *
 * The first argument string \c s must be a string exactly
 * ::UPROC_ALPHABET_SIZE characters long which consists only of uppercase
 * letters. No letter shall appear twice. Passing an unterminated char array
 * results in undefined behaviour.
 */
uproc_alphabet *uproc_alphabet_create(const char *s);


/** Destroy alphabet object */
void uproc_alphabet_destroy(uproc_alphabet *alpha);


/** Translate character to amino acid
 *
 * \param alpha     alphabet object
 * \param c         character to translate
 *
 * \return
 * The value (between 0 and ::UPROC_ALPHABET_SIZE) of the corresponding amino
 * acid or -1 if \c c is a non-alphabetic character.
 *
 * Does not set ::uproc_errno.
 */
uproc_amino uproc_alphabet_char_to_amino(const uproc_alphabet *alpha, int c);


/** Translate amino acid to character
 *
 * \param alpha     alphabet object
 * \param a         amino acid to translate
 *
 * \return
 * Corresponding character, or -1 if `a` is not between 0 and
 * ::UPROC_ALPHABET_SIZE.
 *
 * Does not set ::uproc_errno.
 */
int uproc_alphabet_amino_to_char(const uproc_alphabet *alpha, uproc_amino a);


/** Return the underlying string
 *
 *  Returns a pointer to the underlying string, which is a copy of the argument
 *  passed to uproc_alphabet_create().
 */
const char *uproc_alphabet_str(const uproc_alphabet *alpha);


/**
 * @}
 * @}
 */
#endif

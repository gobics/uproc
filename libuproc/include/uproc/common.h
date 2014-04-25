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

/** \file uproc/common.h
 *
 * Module: \ref grp_intern_common
 *
 * \weakgroup grp_intern
 * \{
 * \weakgroup grp_intern_common
 * \{
 */

#ifndef UPROC_COMMON_H
#define UPROC_COMMON_H

#include <stdint.h>
#include <inttypes.h>
#include <math.h>


/** Epsilon value for comparing floating point numbers */
#define UPROC_EPSILON 1e-5


/** Maks the lowest \c n bits */
#define UPROC_BITMASK(n) (~(~0ULL << (n)))


/** Length of the suffix part of a word */
#define UPROC_PREFIX_LEN 6


/** Length of the prefix part of a word */
#define UPROC_SUFFIX_LEN 12


/** Total word length */
#define UPROC_WORD_LEN (UPROC_PREFIX_LEN + UPROC_SUFFIX_LEN)


/** Type to represent one amino acid. */
typedef int uproc_amino;


/** Bits needed to represent one amino acid */
#define UPROC_AMINO_BITS 5


/** Number of amino acids in the alphabet */
#define UPROC_ALPHABET_SIZE 20


/** Type for prefixes
 *
 * Prefixes are (in contrast to suffixes, see below) contiguous, i.e. all values
 * from 0 to #UPROC_PREFIX_MAX represent valid prefixes.
 */
typedef uint_least32_t uproc_prefix;


/** printf() format specifier */
#define UPROC_PREFIX_PRI PRIu32


/** scanf() format specifier */
#define UPROC_PREFIX_SCN SCNu32


/** Raise \c x to the power of 6 */
#define UPROC_POW6(x) ((x) * (x) * (x) * (x) * (x) * (x))


/** Maximum value of a prefix */
#define UPROC_PREFIX_MAX (UPROC_POW6((unsigned long) UPROC_ALPHABET_SIZE) - 1)


/** Type for suffixes
 *
 * Suffixes are represented as a "bit string" of #UPROC_SUFFIX_LEN amino acids,
 * each represented #UPROC_AMINO_BITS bits.
 */
typedef uint_least64_t uproc_suffix;


/** printf() format for suffixes */
#define UPROC_SUFFIX_PRI PRIu64


/** scanf() format for suffixes */
#define UPROC_SUFFIX_SCN SCNu64


/** Identifier of a protein family */
typedef uint_least16_t uproc_family;


/** Maximum value for #uproc_family */
#define UPROC_FAMILY_MAX (UINT_LEAST16_MAX - 1)


/** Denotes an invalid protein family */
#define UPROC_FAMILY_INVALID (UINT_LEAST16_MAX)


/** printf() format for #uproc_family */
#define UPROC_FAMILY_PRI PRIu16


/** scanf() format for #uproc_family */
#define UPROC_FAMILY_SCN SCNu16


/** Taxonomic ID */
typedef uint_least32_t uproc_tax;


#define UPROC_TAX_PRI PRIuLEAST32
#define UPROC_TAX_SCN SCNuLEAST32

/**
 * \}
 * \}
 */
#endif

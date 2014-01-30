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

/** \file uproc/codon.h
 *
 * Operations on tri-nucleotide codons
 *
 */

#ifndef UPROC_CODON_H
#define UPROC_CODON_H

#include <stdbool.h>

#include "uproc/common.h"


/** Type for nucleotides
 *
 * This type can represent a standard base (A, C, G, T/U) or one of the
 * IUPAC wildcard characters (http://www.bioinformatics.org/sms/iupac.html)
 * except gaps. It is defined as a signed integer, so negative values can be
 * used as return values in case of an error.
 */
typedef int uproc_nt;


/** Nucleotide values */
enum {
    /** Adenine */
    UPROC_NT_A = (1 << 0),

    /** Cytosine */
    UPROC_NT_C = (1 << 1),

    /** Guanine */
    UPROC_NT_G = (1 << 2),

    /** Thymine */
    UPROC_NT_T = (1 << 3),

    /** Uracil */
    UPROC_NT_U = UPROC_NT_T,

    /** Wildcard matching any base */
    UPROC_NT_N = (UPROC_NT_A | UPROC_NT_C | UPROC_NT_G | UPROC_NT_T),

    /** Result of converting a non-alphabetic ([A-Za-z]) character */
    UPROC_NT_NOT_CHAR = -1,

    /** Result of converting a non-IUPAC symbol */
    UPROC_NT_NOT_IUPAC = -2,
};


/** Number of bits needed to represent a valid nucleotide */
#define UPROC_NT_BITS 4


/** Type used to represent a codon (or codon mask)
 *
 * This type needs to be able to store at least ::UPROC_CODON_BITS bits.
 */
typedef unsigned uproc_codon;


/** Number of nucleotides in a codon */
#define UPROC_CODON_NTS 3


/** Bits used to represent a codon */
#define UPROC_CODON_BITS (UPROC_CODON_NTS * UPROC_NT_BITS)


/** Number of codons consisting of only A, C, G and T/U */
#define UPROC_CODON_COUNT (4 * 4 * 4)


/** Number of all possible binary representations of a codon mask */
#define UPROC_BINARY_CODON_COUNT (1 << UPROC_CODON_BITS)


/** Append nucleotide to codon
 *
 *  Similar to uproc_word_append():
 *    `append(ACG, T) == CGT`
 *
 * \param codon     codon to append to
 * \param nt        nucleotide to append
 */
void uproc_codon_append(uproc_codon *codon, uproc_nt nt);


/** Prepend nucleotide to codon
 *
 * Complementary to the append operation, e.g.
 *   `prepend(ACG, T) == TAC`.
 *
 * \param codon     codon to append to
 * \param nt        nucleotide to append
 */
void uproc_codon_prepend(uproc_codon *codon, uproc_nt nt);


/** Retrieve a codon's nucleotide at a certain position
 *
 * \param codon     codon to extract nt from
 * \param position  position of the desired nt
 *
 * \return Nucleotide at the given position or 0 if `position` is >=
 * #UPROC_CODON_NTS
 */
uproc_nt uproc_codon_get_nt(uproc_codon codon, unsigned position);


/** Match a codon against a "codon mask"
 *
 * \param codon     codon to match
 * \param mask      mask to match against
 *
 * \return Whether `codon` matches `mask`.
 */
bool uproc_codon_match(uproc_codon codon, uproc_codon mask);

#endif

/** \file uproc/codon.h
 *
 * Operations on tri-nucleotide codons
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

#ifndef UPROC_CODON_H
#define UPROC_CODON_H

#include "uproc/common.h"

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
 * \return Nucleotide at the given position or 0 if `position` is >= #UPROC_CODON_NTS
 */
uproc_nt uproc_codon_get_nt(uproc_codon codon, unsigned position);

/** Match a codon against a "codon mask"
 *
 * \param codon     codon to match
 * \param mask      mask to match against
 *
 * \return Whether `codon` matches `mask`
 */
bool uproc_codon_match(uproc_codon codon, uproc_codon mask);

#endif

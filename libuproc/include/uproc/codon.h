#ifndef UPROC_CODON_H
#define UPROC_CODON_H

/** \file uproc/codon.h
 *
 * Operations on tri-nucleotide codons
 */

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

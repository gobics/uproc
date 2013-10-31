#ifndef UPRO_CODON_H
#define UPRO_CODON_H

/** \file upro/codon.h
 *
 * Operations on tri-nucleotide codons
 */

#include "upro/common.h"

/** Append nucleotide to codon
 *
 *  Similar to upro_word_append():
 *    `append(ACG, T) == CGT`
 *
 * \param codon     codon to append to
 * \param nt        nucleotide to append
 */
void upro_codon_append(upro_codon *codon, upro_nt nt);

/** Prepend nucleotide to codon
 *
 * Complementary to the append operation, e.g.
 *   `prepend(ACG, T) == TAC`.
 *
 * \param codon     codon to append to
 * \param nt        nucleotide to append
 */
void upro_codon_prepend(upro_codon *codon, upro_nt nt);

/** Retrieve a codon's nucleotide at a certain position
 *
 * \param codon     codon to extract nt from
 * \param position  position of the desired nt
 *
 * \return Nucleotide at the given position or 0 if `position` is >= #UPRO_CODON_NTS
 */
upro_nt upro_codon_get_nt(upro_codon codon, unsigned position);

/** Match a codon against a "codon mask"
 *
 * \param codon     codon to match
 * \param mask      mask to match against
 *
 * \return Whether `codon` matches `mask`
 */
bool upro_codon_match(upro_codon codon, upro_codon mask);

#endif

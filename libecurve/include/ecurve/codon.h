#ifndef EC_CODON_H
#define EC_CODON_H

/** \file ecurve/codon.h
 *
 * Operations on tri-nucleotide codons
 */

#include "ecurve/common.h"

/** Append nucleotide to codon
 *
 *  Similar to ec_word_append():
 *    `append(ACG, T) == CGT`
 *
 * \param codon     codon to append to
 * \param nt        nucleotide to append
 */
void ec_codon_append(ec_codon *codon, ec_nt nt);

/** Prepend nucleotide to codon
 *
 * Complementary to the append operation, e.g.
 *   `prepend(ACG, T) == TAC`.
 *
 * \param codon     codon to append to
 * \param nt        nucleotide to append
 */
void ec_codon_prepend(ec_codon *codon, ec_nt nt);

/** Retrieve a codon's nucleotide at a certain position
 *
 * \param codon     codon to extract nt from
 * \param position  position of the desired nt
 *
 * \return Nucleotide at the given position or 0 if `position` is >= #EC_CODON_NTS
 */
ec_nt ec_codon_get_nt(ec_codon codon, unsigned position);

/** Match a codon against a "codon mask"
 *
 * \param codon     codon to match
 * \param mask      mask to match against
 *
 * \return Whether `codon` matches `mask`
 */
bool ec_codon_match(ec_codon codon, ec_codon mask);

#endif

#ifndef UPROC_SUBSTMAT_H
#define UPROC_SUBSTMAT_H

/** \file uproc/substmat.h
 *
 * Obtain alignment distances between amino acids
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include "uproc/common.h"
#include "uproc/io.h"
#include "uproc/alphabet.h"

/** Matrix of amino acid distances */
struct uproc_substmat {
    /** Matrix containing distances */
    double dists[UPROC_SUFFIX_LEN][UPROC_ALPHABET_SIZE << UPROC_AMINO_BITS];
};

/** Calculate index for given amino acids x, y
 *
 * Don't use this, it may change in the future. Use uproc_substmat_get() and
 * uproc_substmat_set().
 */
#define UPROC_SUBSTMAT_INDEX(x, y) ((x) << UPROC_AMINO_BITS | (y))


/** Initialize a distance matrix object to zeroes
 *
 * \param mat   distance matrix object to initialize
 *
 * \retval #UPROC_FAILURE  an error occured
 * \retval #UPROC_SUCCESS  else
 */
int uproc_substmat_init(struct uproc_substmat *mat);

/** Get distance of two amino acids
 *
 * Retrieves distance between two amino acids `x` and `y`.
 *
 * \param mat   distance matrix
 * \param x     amino acid
 * \param y     another amino acid
 *
 * \return  distance between x and y
 */
double uproc_substmat_get(const struct uproc_substmat *mat, unsigned pos,
        uproc_amino x, uproc_amino y);

/** Set distance of two amino acids
 *
 * Sets distance between `x` and `y` to `dist`.
 *
 * \param mat   distance matrix
 * \param x     amino acid
 * \param y     another amino acid
 * \param dist  distance between x and y
 *
 * \retval #UPROC_FAILURE  an error occured
 * \retval #UPROC_SUCCESS  else
 */
void uproc_substmat_set(struct uproc_substmat *mat, unsigned pos, uproc_amino x,
        uproc_amino y, double dist);

/** Load substition matrix from a file.
 *
 * \param mat   pointer to the first of `n` distance matrices
 * \param iotype    IO type, see #uproc_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPROC_FAILURE  an error occured
 * \retval #UPROC_SUCCESS  else
 */
int uproc_substmat_load(struct uproc_substmat *mat, enum uproc_io_type iotype,
        const char *pathfmt, ...);

int uproc_substmat_loadv(struct uproc_substmat *mat, enum uproc_io_type iotype,
        const char *pathfmt, va_list ap);
#endif

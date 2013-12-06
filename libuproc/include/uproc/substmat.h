/** \file uproc/substmat.h
 *
 * Compute alignment distances between amino acid words
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

#ifndef UPROC_SUBSTMAT_H
#define UPROC_SUBSTMAT_H

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
void uproc_substmat_set(struct uproc_substmat *mat, unsigned pos,
                        uproc_amino x, uproc_amino y, double dist);

/** Look up distances between amino acids of a suffix
 *
 * \param mat   substitution matrix
 * \param s1    first suffix
 * \param s2    second suffix
 * \param dist  _OUT_: array containing distance of each amino acid pair
 */
void uproc_substmat_align_suffixes(const struct uproc_substmat *mat,
                                   uproc_suffix s1, uproc_suffix s2,
                                   double dist[static UPROC_SUFFIX_LEN]);

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

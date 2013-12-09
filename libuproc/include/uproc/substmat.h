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
typedef struct uproc_substmat_s uproc_substmat;


/** Create substitution matrix */
uproc_substmat *uproc_substmat_create(void);

/** Destroy substitution matrix */
void uproc_substmat_destroy(uproc_substmat *mat);

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
double uproc_substmat_get(const uproc_substmat *mat, unsigned pos,
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
void uproc_substmat_set(uproc_substmat *mat, unsigned pos, uproc_amino x,
                        uproc_amino y, double dist);

/** Look up distances between amino acids of a suffix
 *
 * \param mat   substitution matrix
 * \param s1    first suffix
 * \param s2    second suffix
 * \param dist  _OUT_: array containing distance of each amino acid pair
 */
void uproc_substmat_align_suffixes(const uproc_substmat *mat, uproc_suffix s1,
                                   uproc_suffix s2,
                                   double dist[static UPROC_SUFFIX_LEN]);

/** Load substition matrix from a file */
uproc_substmat *uproc_substmat_load(enum uproc_io_type iotype,
                                    const char *pathfmt, ...);

uproc_substmat *uproc_substmat_loadv(enum uproc_io_type iotype,
                                     const char *pathfmt, va_list ap);
#endif

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

/** \file uproc/substmat.h
 *
 * Module: \ref grp_datastructs_substmat
 *
 * \weakgroup grp_datastructs
 * \{
 * \weakgroup grp_datastructs_substmat
 * \{
 */

#ifndef UPROC_SUBSTMAT_H
#define UPROC_SUBSTMAT_H

#include <stdio.h>
#include <stdarg.h>
#include "uproc/common.h"
#include "uproc/io.h"
#include "uproc/alphabet.h"

/** \defgroup obj_substmat object uproc_substmat
 *
 * Array of matrices of amino acid distances
 *
 * An array of ::UPROC_SUFFIX_LEN matrices used to obtain pairwise distances of
 * the suffix part of two ::uproc_word instances.
 *
 * \{
 */

/** \struct uproc_substmat
 * \copybrief obj_substmat
 *
 * See \ref obj_substmat for details.
 */
typedef struct uproc_substmat_s uproc_substmat;

/** Create substmat object
 *
 * Create substitution matrix object with all entries set to 0.
 */
uproc_substmat *uproc_substmat_create(void);

/** Destroy substmat object */
void uproc_substmat_destroy(uproc_substmat *mat);

/** Create identity substitution matrix */
uproc_substmat *uproc_substmat_eye(void);

/** Get distance of two amino acids
 *
 * Retrieves distance between two amino acids \c x and \c y at the suffix
 * position \c pos
 *
 * \param mat   distance matrix
 * \param pos   position (between 0 and ::UPROC_SUFFIX_LEN)
 * \param x     amino acid
 * \param y     another amino acid
 *
 * \return  distance between x and y
 */
double uproc_substmat_get(const uproc_substmat *mat, unsigned pos,
                          uproc_amino x, uproc_amino y);

/** Set distance of two amino acids
 *
 * Sets the distance between two amino acids \c x and \c y at the suffix
 * position \c pos to \c dist.
 *
 * \param mat   distance matrix
 * \param pos   position (between 0 and ::UPROC_SUFFIX_LEN)
 * \param x     amino acid
 * \param y     another amino acid
 * \param dist  distance between x and y
 */
void uproc_substmat_set(uproc_substmat *mat, unsigned pos, uproc_amino x,
                        uproc_amino y, double dist);

/** Look up all distances between amino acids in a suffix
 *
 * \c dist must be a pointer into an array with at least ::UPROC_SUFFIX_LEN
 * elements.
 *
 * \param mat   substitution matrix
 * \param s1    first suffix
 * \param s2    second suffix
 * \param dist  _OUT_: array containing distance of each amino acid pair
 *
 */
void uproc_substmat_align_suffixes(const uproc_substmat *mat, uproc_suffix s1,
                                   uproc_suffix s2, double *dist);

/** Load substmat from file
 *
 * \param iotype    IO type, see ::uproc_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 */
uproc_substmat *uproc_substmat_load(enum uproc_io_type iotype,
                                    const char *pathfmt, ...);

/** Load substmat from file
 *
 * Like ::uproc_substmat_load, but with a \c va_list instead of a variable
 * number of arguments.
 */
uproc_substmat *uproc_substmat_loadv(enum uproc_io_type iotype,
                                     const char *pathfmt, va_list ap);

/** Store substitution matrix to stream */
int uproc_substmat_stores(const uproc_substmat *mat, uproc_io_stream *stream);

/** Store substitution matrix to file */
int uproc_substmat_store(const uproc_substmat *mat, enum uproc_io_type iotype,
                       const char *pathfmt, ...);

/** Store substitution matrix to file
 *
 * Like ::uproc_substmat_store, but with a \c va_list instead of a variable
 * number of arguments.
 */
int uproc_substmat_storev(const uproc_substmat *mat, enum uproc_io_type iotype,
                        const char *pathfmt, va_list ap);
/** \} */

/**
 * \}
 * \}
 */
#endif

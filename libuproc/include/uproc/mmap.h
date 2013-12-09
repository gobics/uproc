/** \file mmap.h
 *
 * Map a file to an ecurve
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

#ifndef UPROC_MMAP_H
#define UPROC_MMAP_H

#include <stdarg.h>
#include "uproc/ecurve.h"

/** Map a file to an ecurve
 *
 * The file must have been created by `uproc_mmap_store()`, preferably on
 * the same machine.
 *
 * \param ecurve    ecurve to be mapped
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPROC_ESYSCALL     an error occured while calling an OS function
 * \retval #UPROC_EINVAL       malformed file
 * \retval #UPROC_SUCCESS      else
 */
uproc_ecurve *uproc_mmap_map(const char *pathfmt, ...);

uproc_ecurve *uproc_mmap_mapv(const char *pathfmt, va_list ap);

/** Release mapping and close the underlying file descriptor
 *
 * \param ecurve    ecurve mapped with `uproc_mmap_map()`
 */
void uproc_mmap_unmap(uproc_ecurve *ecurve);

/** Store ecurve in a format suitable for `uproc_mmap_map()`
 *
 * \param ecurve    ecurve to store
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPROC_ESYSCALL     an error occured while calling an OS function
 * \retval #UPROC_SUCCESS      else
 */
int uproc_mmap_store(const uproc_ecurve *ecurve, const char *pathfmt, ...);

int uproc_mmap_storev(const uproc_ecurve *ecurve, const char *pathfmt,
                      va_list ap);

#endif

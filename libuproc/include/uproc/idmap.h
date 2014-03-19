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

/** \file uproc/idmap.h
 *
 * Module: \ref grp_datastructs_idmap
 *
 * \weakgroup grp_datastructs
 * \{
 * \weakgroup grp_datastructs_idmap
 * \{
 */

#ifndef UPROC_IDMAP_H
#define UPROC_IDMAP_H

#include "uproc/common.h"
#include "uproc/io.h"


/** \defgroup obj_idmap object uproc_idmap
 *
 * Map between protein family name and its numeric identifier
 *
 * \{
 */

/** \struct uproc_idmap
 * \copybrief obj_idmap
 *
 * See \ref obj_idmap for details.
 */
typedef struct uproc_idmap_s uproc_idmap;


/** Create idmap object */
uproc_idmap *uproc_idmap_create(void);


/** Destroy idmap object */
void uproc_idmap_destroy(uproc_idmap *map);


/** Get family number
 *
 * Returns the family number for \c name.
 *
 * If needed, a copy of \c name is inserted into the map. If the number of
 * families reaches ::UPROC_FAMILY_MAX, no more names can be added.
 *
 * This is currently implemented as linear search, so it will slow down with
 * a growing number of entries.
 *
 * \return
 * Returns a number in <tt>[0, ::UPROC_FAMILY_MAX]</tt> that maps to \c name,
 * or ::UPROC_FAMILY_INVALID if an error occurs or the limit was reached while
 * trying to insert a new name.
 */
uproc_family uproc_idmap_family(uproc_idmap *map, const char *name);


/** Get family string
 *
 * Returns the family name associated with the family number \c family.
 * If there is none, returns NULL.
 *
 * Modifying the returned string will affect the stored value.
 */
char *uproc_idmap_str(const uproc_idmap *map, uproc_family family);


/** Load idmap from stream */
uproc_idmap *uproc_idmap_loads(uproc_io_stream *stream);


/** Load idmap from file
 *
 * \param iotype    IO type, see ::uproc_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 */
uproc_idmap *uproc_idmap_load(enum uproc_io_type iotype,
                              const char *pathfmt, ...);


/** Load idmap from file
 *
 * Like ::uproc_idmap_load, but with a \c va_list instead of a variable
 * number of arguments.
 */
uproc_idmap *uproc_idmap_loadv(enum uproc_io_type iotype,
                               const char *pathfmt, va_list ap);


/** Store idmap to stream */
int uproc_idmap_stores(const uproc_idmap *map, uproc_io_stream *stream);


/** Store idmap to file */
int uproc_idmap_storev(const uproc_idmap *map, enum uproc_io_type iotype,
                       const char *pathfmt, va_list ap);


/** Store idmap to file
 *
 * Like ::uproc_idmap_store, but with a \c va_list instead of a variable
 * number of arguments.
 */
int uproc_idmap_store(const uproc_idmap *map, enum uproc_io_type iotype,
                      const char *pathfmt, ...);

/** \} */

/**
 * \}
 * \}
 */
#endif

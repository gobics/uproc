/** \file uproc/idmap.h
 * Map string identifier to family number
 *
 * Error handling facilities
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

#ifndef UPROC_IDMAP_H
#define UPROC_IDMAP_H

#include "uproc/common.h"
#include "uproc/io.h"

typedef struct uproc_idmap_s uproc_idmap;

uproc_idmap *uproc_idmap_create(void);

void uproc_idmap_destroy(uproc_idmap *map);

uproc_family uproc_idmap_family(uproc_idmap *map, const char *s);

const char *uproc_idmap_str(const uproc_idmap *map, uproc_family family);

uproc_idmap *uproc_idmap_loads(uproc_io_stream *stream);

uproc_idmap *uproc_idmap_loadv(enum uproc_io_type iotype,
                      const char *pathfmt, va_list ap);

uproc_idmap *uproc_idmap_load(enum uproc_io_type iotype,
                     const char *pathfmt, ...);

int uproc_idmap_stores(const uproc_idmap *map, uproc_io_stream *stream);


int uproc_idmap_storev(const uproc_idmap *map, enum uproc_io_type iotype,
                       const char *pathfmt, va_list ap);

int uproc_idmap_store(const uproc_idmap *map, enum uproc_io_type iotype,
                      const char *pathfmt, ...);
#endif

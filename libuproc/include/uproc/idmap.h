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

#define UPROC_IDMAP_MAXLEN 64

struct uproc_idmap
{
    uproc_family n;
    char *s[UPROC_FAMILY_MAX];
};

#define UPROC_IDMAP_INITIALIZER { 0, { NULL } }

int uproc_idmap_init(struct uproc_idmap *map);

void uproc_idmap_destroy(struct uproc_idmap *map);

uproc_family uproc_idmap_family(struct uproc_idmap *map, const char *s);

const char *uproc_idmap_str(const struct uproc_idmap *map, uproc_family family);

int uproc_idmap_loads(struct uproc_idmap *map, uproc_io_stream *stream);

int uproc_idmap_loadv(struct uproc_idmap *map, enum uproc_io_type iotype,
                      const char *pathfmt, va_list ap);

int uproc_idmap_load(struct uproc_idmap *map, enum uproc_io_type iotype,
                     const char *pathfmt, ...);

int uproc_idmap_stores(const struct uproc_idmap *map, uproc_io_stream *stream);


int uproc_idmap_storev(const struct uproc_idmap *map, enum uproc_io_type iotype,
                       const char *pathfmt, va_list ap);

int uproc_idmap_store(const struct uproc_idmap *map, enum uproc_io_type iotype,
                      const char *pathfmt, ...);
#endif

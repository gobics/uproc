/** \file uproc/storage.h
 *
 * Load/store ecurves from/to files
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

#ifndef UPROC_STORAGE_H
#define UPROC_STORAGE_H


#include <stdio.h>
#include <stdarg.h>
#include "uproc/io.h"
#include "uproc/ecurve.h"

/** Values to use as the `format` argument */
enum uproc_ecurve_format
{
    UPROC_STORAGE_PLAIN,
    UPROC_STORAGE_BINARY,
};

uproc_ecurve *uproc_ecurve_loads(enum uproc_ecurve_format format,
                                 uproc_io_stream *stream);

/** Load ecurve from a file
 *
 * Opens a file for reading, allocates a new #uproc_ecurve object and parses
 * the data in the file using the given format.
 *
 * \param ecurve    pointer to ecurve to which the data will be loaded into
 * \param format    format to use, see #uproc_ecurve_format
 * \param iotype    IO type, see #uproc_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPROC_FAILURE  an error occured
 * \retval #UPROC_SUCCESS  else
 */
uproc_ecurve *uproc_ecurve_load(enum uproc_ecurve_format format,
                                enum uproc_io_type iotype,
                                const char *pathfmt, ...);

uproc_ecurve *uproc_ecurve_loadv(enum uproc_ecurve_format format,
                                 enum uproc_io_type iotype,
                                 const char *pathfmt, va_list ap);

int uproc_ecurve_stores(const uproc_ecurve *ecurve,
                        enum uproc_ecurve_format format,
                        uproc_io_stream *stream);

/** Store ecurve to a file
 *
 * Stores an #uproc_ecurve object to a file using the given format.
 *
 * \param ecurve    pointer to ecurve to store
 * \param format    format to use, see #uproc_ecurve_format
 * \param iotype    IO type, see #uproc_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPROC_FAILURE  an error occured
 * \retval #UPROC_SUCCESS  else
 */
int uproc_ecurve_store(const uproc_ecurve *ecurve,
                       enum uproc_ecurve_format format,
                       enum uproc_io_type iotype, const char *pathfmt, ...);

int uproc_ecurve_storev(const uproc_ecurve *ecurve,
                        enum uproc_ecurve_format format,
                        enum uproc_io_type iotype, const char *pathfmt,
                        va_list ap);
#endif

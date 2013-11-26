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
enum uproc_storage_format {
    UPROC_STORAGE_PLAIN,
    UPROC_STORAGE_BINARY,
};

/** Load ecurve from a file
 *
 * Opens a file for reading, allocates a new #uproc_ecurve object and parses
 * the data in the file using the given format.
 *
 * \param ecurve    pointer to ecurve to which the data will be loaded into
 * \param format    format to use, see #uproc_storage_format
 * \param iotype    IO type, see #uproc_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPROC_FAILURE  an error occured
 * \retval #UPROC_SUCCESS  else
 */
int uproc_storage_load(struct uproc_ecurve *ecurve,
                       enum uproc_storage_format format,
                       enum uproc_io_type iotype, const char *pathfmt, ...);

int uproc_storage_loadv(struct uproc_ecurve *ecurve,
                        enum uproc_storage_format format,
                        enum uproc_io_type iotype, const char *pathfmt,
                        va_list ap);

/** Store ecurve to a file
 *
 * Stores an #uproc_ecurve object to a file using the given format.
 *
 * \param ecurve    pointer to ecurve to store
 * \param format    format to use, see #uproc_storage_format
 * \param iotype    IO type, see #uproc_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPROC_FAILURE  an error occured
 * \retval #UPROC_SUCCESS  else
 */
int uproc_storage_store(const struct uproc_ecurve *ecurve,
                        enum uproc_storage_format format,
                        enum uproc_io_type iotype, const char *pathfmt, ...);

int uproc_storage_storev(const struct uproc_ecurve *ecurve,
                         enum uproc_storage_format format,
                         enum uproc_io_type iotype, const char *pathfmt,
                         va_list ap);
#endif

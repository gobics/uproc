#ifndef UPROC_STORAGE_H
#define UPROC_STORAGE_H

/** \file uproc/storage.h
 *
 * Load and store ecurves from/to files.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include "uproc/io.h"
#include "uproc/ecurve.h"

/** Values to use as the `format` argument */
enum uproc_storage_format {
    UPROC_STORAGE_PLAIN,
    UPROC_STORAGE_BINARY,
    UPROC_STORAGE_MMAP,
};

/** Load ecurve from a file
 *
 * Opens a file for reading, allocates a new #uproc_ecurve object and parses the
 * data in the file using the given format.
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
        enum uproc_storage_format format, enum uproc_io_type iotype,
        const char *pathfmt, ...);

int uproc_storage_loadv(struct uproc_ecurve *ecurve,
        enum uproc_storage_format format, enum uproc_io_type iotype,
        const char *pathfmt, va_list ap);

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
        enum uproc_storage_format format, enum uproc_io_type iotype,
        const char *pathfmt, ...);

int uproc_storage_storev(const struct uproc_ecurve *ecurve,
        enum uproc_storage_format format, enum uproc_io_type iotype,
        const char *pathfmt, va_list ap);
#endif

#ifndef UPRO_STORAGE_H
#define UPRO_STORAGE_H

/** \file upro/storage.h
 *
 * Load and store ecurves from/to files.
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include "upro/io.h"
#include "upro/ecurve.h"

/** Values to use as the `format` argument */
enum upro_storage_format {
    UPRO_STORAGE_PLAIN,
    UPRO_STORAGE_BINARY,
    UPRO_STORAGE_MMAP,
};

/** Load ecurve from a file
 *
 * Opens a file for reading, allocates a new #upro_ecurve object and parses the
 * data in the file using the given format.
 *
 * \param ecurve    pointer to ecurve to which the data will be loaded into
 * \param format    format to use, see #upro_storage_format
 * \param iotype    IO type, see #upro_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPRO_FAILURE  an error occured
 * \retval #UPRO_SUCCESS  else
 */
int upro_storage_load(struct upro_ecurve *ecurve,
        enum upro_storage_format format, enum upro_io_type iotype,
        const char *pathfmt, ...);

int upro_storage_loadv(struct upro_ecurve *ecurve,
        enum upro_storage_format format, enum upro_io_type iotype,
        const char *pathfmt, va_list ap);

/** Store ecurve to a file
 *
 * Stores an #upro_ecurve object to a file using the given format.
 *
 * \param ecurve    pointer to ecurve to store
 * \param format    format to use, see #upro_storage_format
 * \param iotype    IO type, see #upro_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPRO_FAILURE  an error occured
 * \retval #UPRO_SUCCESS  else
 */
int upro_storage_store(const struct upro_ecurve *ecurve,
        enum upro_storage_format format, enum upro_io_type iotype,
        const char *pathfmt, ...);

int upro_storage_storev(const struct upro_ecurve *ecurve,
        enum upro_storage_format format, enum upro_io_type iotype,
        const char *pathfmt, va_list ap);
#endif

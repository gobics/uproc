#ifndef UPRO_STORAGE_H
#define UPRO_STORAGE_H

/** \file upro/storage.h
 *
 * Load and store ecurves from/to files.
 *
 */

#include <stdio.h>
#include "upro/io.h"
#include "upro/ecurve.h"

/** Values to use as the `format` argument */
enum upro_storage_format {
    UPRO_STORAGE_PLAIN,
    UPRO_STORAGE_BINARY,
    UPRO_STORAGE_MMAP,
};

/** Flags */
enum upro_storage_flags {
    /** Use gzip compression for writing (if compiled with zlib) */
    UPRO_STORAGE_GZIP = (1 << 0),
};

/** Load ecurve from a file
 *
 * Opens a file for reading, allocates a new #upro_ecurve object and parses the
 * data in the file using the given format.
 *
 * \param ecurve    pointer to ecurve to which the data will be loaded into
 * \param path      file path
 * \param format    format to use, must be one of the values defined in #upro_storage_format
 *
 * \retval #UPRO_FAILURE  an error occured
 * \retval #UPRO_SUCCESS  else
 */
int upro_storage_load(struct upro_ecurve *ecurve, const char *path,
        enum upro_storage_format format, enum upro_io_type iotype);

/** Store ecurve to a file
 *
 * Stores an #upro_ecurve object to a file using the given format.
 *
 * \param ecurve    pointer to ecurve to store
 * \param path      file path
 * \param format    format to use, see #upro_storage_format
 * \param iotype    IO type, see #upro_io_type
 *
 * \retval #UPRO_FAILURE  an error occured
 * \retval #UPRO_SUCCESS  else
 */
int upro_storage_store(const struct upro_ecurve *ecurve, const char *path,
        enum upro_storage_format format, enum upro_io_type iotype);

#endif

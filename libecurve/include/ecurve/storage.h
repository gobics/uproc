#ifndef EC_STORAGE_H
#define EC_STORAGE_H

/** \file ecurve/storage.h
 *
 * Load and store ecurves from/to files.
 *
 */

#include <stdio.h>
#include "ecurve/ecurve.h"

/** Values to use as the `format` argument */
enum ec_storage_format {
    EC_STORAGE_PLAIN,
    EC_STORAGE_BINARY,
    EC_STORAGE_MMAP,
};

/** Flags */
enum ec_storage_flags {
    /** Use gzip compression for writing (if compiled with zlib) */
    EC_STORAGE_GZIP = (1 << 0),
};

/** Load ecurve from a file
 *
 * Opens a file for reading, allocates a new #ec_ecurve object and parses the
 * data in the file using the given format.
 *
 * \param ecurve    pointer to ecurve to which the data will be loaded into
 * \param path      file path
 * \param format    format to use, must be one of the values defined in #ec_storage_format
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
int ec_storage_load(struct ec_ecurve *ecurve, const char *path, int format);

/** Store ecurve to a file
 *
 * Stores an #ec_ecurve object to a file using the given format.
 *
 * \param ecurve    pointer to ecurve to store
 * \param path      file path
 * \param format    format to use, must be one of the values defined in #ec_storage_format
 * \param flags     bitwise OR of values defined in #ec_storage_flags
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
int ec_storage_store(const struct ec_ecurve *ecurve, const char *path,
                     int format, int flags);

#endif

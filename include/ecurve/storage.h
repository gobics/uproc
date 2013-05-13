#ifndef EC_STORAGE_H
#define EC_STORAGE_H

/** \file ecurve/storage.h
 *
 * Load and store ecurves from/to files.
 *
 * For loading an ecurve, you can use
 *      - ec_ecurve_load() with one of the `ec_ecurve_load_*()` functions
 *      - open a FILE stream yourself and use one of the `ec_ecurve_load_*()` functions.
 *
 * And likewise for storing them (substitute "load" for "store").
 */

#include <stdio.h>
#include "ecurve/ecurve.h"

/** Load ecurve from a file
 *
 * Opens a file for reading, allocates a new #ec_ecurve object and parses the
 * data in the file using the `load` function, i.e. the choice for the `load`
 * function determines the file format.
 *
 * \param path      file path
 * \param ecurve    pointer to ecurve to which the data will be loaded into
 * \param load      function to parse the data
 */
int ec_ecurve_load(ec_ecurve *ecurve, const char *path,
                   int (*load)(ec_ecurve *, FILE *));

/** Store ecurve to a file
 *
 * Similar to ec_ecurve_load(), takes a `store` function that determines the
 * file format.
 *
 * \param path      file path
 * \param ecurve    pointer to ecurve to store
 * \param store     function to parse the data
 */
int ec_ecurve_store(const ec_ecurve *ecurve, const char *path,
                    int (*store)(const ec_ecurve *, FILE *));

/** Load ecurve in binary format */
int ec_ecurve_load_binary(ec_ecurve *ecurve, FILE *stream);

/** Store ecurve in binary format */
int ec_ecurve_store_binary(const ec_ecurve *ecurve, FILE *stream);

/** Load ecurve in plain text format */
int ec_ecurve_load_plain(ec_ecurve *ecurve, FILE *stream);

/** Store ecurve in plain text format */
int ec_ecurve_store_plain(const ec_ecurve *ecurve, FILE *stream);

#endif

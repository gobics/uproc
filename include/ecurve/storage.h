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
 * function determines the file format. The `arg` parameter is specific to the
 * `load` function and shall be chosen accordingly.
 * 
 * \param path      file path
 * \param ecurve    pointer to ecurve to which the data will be loaded into
 * \param load      function to parse the data
 * \param arg       argument to `load`
 */
int ec_ecurve_load(ec_ecurve *ecurve, const char *path,
                   int (*load)(ec_ecurve *, FILE*, void *), void *arg);

/** Store ecurve to a file
 *
 * Similar to ec_ecurve_load(), takes a `store` function that determines the
 * file format.
 *
 * \param path      file path
 * \param ecurve    pointer to ecurve to store
 * \param store     function to parse the data
 * \param arg       argument to `store`
 */
int ec_ecurve_store(const ec_ecurve *ecurve, const char *path,
                    int (*store)(const ec_ecurve *, FILE *, void *), void *arg);

/** Load ecurve in binary format
 *
 * Loads ecurve in binary format from `stream` into `ecurve`.
 *
 * `arg` is ignored.
 */
int ec_ecurve_load_binary(ec_ecurve *ecurve, FILE *stream, void *arg);

/** Store ecurve in binary format
 *
 * Stores `ecurve` to `stream` using a binary format.
 *
 * `arg` is ignored.
 */
int ec_ecurve_store_binary(const ec_ecurve *ecurve, FILE *stream, void *arg);

/** Load ecurve in plain text format
 *
 * Loads ecurve in plain text from `stream` into `ecurve`.
 *
 * `arg` shall be a pointer to an #ec_alphabet to use for translation
 */
int ec_ecurve_load_plain(ec_ecurve *ecurve, FILE *stream, void *arg);

/** Store ecurve in plain text format
 *
 * Stores `ecurve` to `stream` in plain text.
 *
 * `arg` shall be a pointer to an #ec_alphabet to use for translation
 */
int ec_ecurve_store_plain(const ec_ecurve *ecurve, FILE *stream, void *arg);

#endif 

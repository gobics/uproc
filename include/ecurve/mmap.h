#ifndef EC_MMAP_H
#define EC_MMAP_H

/** \file mmap.h 
 *
 * Directly map a file to an ecurve using POSIX' mmap().
 *
 * While this probably improves loading time compared to using
 * `ec_ecurve_load()`, `mmap()`able files are not portable between
 * different architectures (or even machines).
 */

#include "ecurve/ecurve.h"

/** Map a file to an ecurve
 *
 * The file must have been created by using `ec_mmap_store()`, preferably on
 * the same machine.
 *
 * \param ecurve    ecurve to be mapped
 * \param path      file path
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
int ec_mmap_map(ec_ecurve *ecurve, const char *path);

/** Release mapping and close the underlying file descriptor
 *
 * \param ecurve    ecurve mapped with `ec_mmap_map()`
 */
void ec_mmap_unmap(ec_ecurve *ecurve);

/** Store ecurve in a format suitable for `ec_mmap_map()`
 *
 * \param ecurve    ecurve to store
 * \param path      file path
 *
 * \retval #EC_FAILURE  an error occured
 * \retval #EC_SUCCESS  else
 */
int ec_mmap_store(const ec_ecurve *ecurve, const char *path);

#endif

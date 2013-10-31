#ifndef UPRO_MMAP_H
#define UPRO_MMAP_H

/** \file mmap.h
 *
 * Directly map a file to an ecurve using POSIX' mmap().
 *
 * While this probably improves loading time compared to using
 * `upro_ecurve_load()`, `mmap()`able files are not portable between
 * different architectures (or even machines).
 */

#include "upro/ecurve.h"

/** Map a file to an ecurve
 *
 * The file must have been created by using `upro_mmap_store()`, preferably on
 * the same machine.
 *
 * \param ecurve    ecurve to be mapped
 * \param path      file path
 *
 * \retval #UPRO_ESYSCALL     an error occured while calling an OS function
 * \retval #UPRO_EINVAL       malformed file
 * \retval #UPRO_SUCCESS      else
 */
int upro_mmap_map(struct upro_ecurve *ecurve, const char *path);

/** Release mapping and close the underlying file descriptor
 *
 * \param ecurve    ecurve mapped with `upro_mmap_map()`
 */
void upro_mmap_unmap(struct upro_ecurve *ecurve);

/** Store ecurve in a format suitable for `upro_mmap_map()`
 *
 * \param ecurve    ecurve to store
 * \param path      file path
 *
 * \retval #UPRO_ESYSCALL     an error occured while calling an OS function
 * \retval #UPRO_SUCCESS      else
 */
int upro_mmap_store(const struct upro_ecurve *ecurve, const char *path);

#endif

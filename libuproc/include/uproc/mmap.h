#ifndef UPROC_MMAP_H
#define UPROC_MMAP_H

/** \file mmap.h
 *
 * Directly map a file to an ecurve using POSIX' mmap().
 *
 * While this probably improves loading time compared to using
 * `uproc_ecurve_load()`, `mmap()`able files are not portable between
 * different architectures (or even machines).
 */

#include <stdarg.h>
#include "uproc/ecurve.h"

/** Map a file to an ecurve
 *
 * The file must have been created by `uproc_mmap_store()`, preferably on
 * the same machine.
 *
 * \param ecurve    ecurve to be mapped
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPROC_ESYSCALL     an error occured while calling an OS function
 * \retval #UPROC_EINVAL       malformed file
 * \retval #UPROC_SUCCESS      else
 */
int uproc_mmap_map(struct uproc_ecurve *ecurve, const char *pathfmt, ...);

int uproc_mmap_mapv(struct uproc_ecurve *ecurve, const char *pathfmt, va_list ap);

/** Release mapping and close the underlying file descriptor
 *
 * \param ecurve    ecurve mapped with `uproc_mmap_map()`
 */
void uproc_mmap_unmap(struct uproc_ecurve *ecurve);

/** Store ecurve in a format suitable for `uproc_mmap_map()`
 *
 * \param ecurve    ecurve to store
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPROC_ESYSCALL     an error occured while calling an OS function
 * \retval #UPROC_SUCCESS      else
 */
int uproc_mmap_store(const struct uproc_ecurve *ecurve, const char *pathfmt,
        ...);

int uproc_mmap_storev(const struct uproc_ecurve *ecurve, const char *pathfmt,
        va_list ap);

#endif

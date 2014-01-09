/** \file uproc/ecurve.h
 *
 * Look up protein sequences from an "Evolutionary Curve"
 *
 * Copyright 2014 Peter Meinicke, Robin Martinjak
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

#ifndef UPROC_ECURVE_H
#define UPROC_ECURVE_H

#include <stdio.h>
#include <stdarg.h>

#include "uproc/alphabet.h"
#include "uproc/io.h"
#include "uproc/word.h"

/** Storage format */
enum uproc_ecurve_format
{
    UPROC_STORAGE_PLAIN,
    UPROC_STORAGE_BINARY,
};

/** Lookup return codes */
enum
{
    /** Exact match */
    UPROC_ECURVE_EXACT,

    /** No exact match */
    UPROC_ECURVE_INEXACT,

    /** Out of bounds */
    UPROC_ECURVE_OOB,
};

/** Opaque type for ecurves */
typedef struct uproc_ecurve_s uproc_ecurve;

/** Initialize an empty ecurve object
 *
 * \param alphabet      string to initialize the ecurve's alphabet
 *                      (see uproc_alphabet_init())
 * \param suffix_count  number of entries in the suffix table
 *
 * \return
 * On success, returns a new uproc_ecurve object.
 * On failure, returns NULL and sets ::uproc_errno to ::UPROC_EINVAL
 * and ::uproc_errmsg accordingly.
 */
uproc_ecurve *uproc_ecurve_create(const char *alphabet, size_t suffix_count);

/** Free memory of an ecurve object
 *
 * \param ecurve    ecurve to destroy
 */
void uproc_ecurve_destroy(uproc_ecurve *ecurve);

/* Append one partial ecurve to another
 *
 * They may not overlap, i.e. the last non-empty prefix of `dest` needs to be
 * less than the first non-empty prefix of `src`.
 *
 * `dest` may not be "mmap()ed"
 *
 * \param dest  ecurve to append to
 * \param src   ecurve to append
 *
 * \retval #UPROC_EINVAL   `dest` is mmap()ed or `dest` and `src` overlap
 * \retval #UPROC_ENOMEM   memory allocation failed
 * \retval #UPROC_SUCCESS  else
 */
int uproc_ecurve_append(uproc_ecurve *dest, const uproc_ecurve *src);

/** Find the closest neighbours of a word in the ecurve
 *
 * NOTE: `ecurve` may not be empty.
 *
 * If `word` was found exactly as it it in the ecurve, `#UPROC_ECURVE_EXACT`
 * will be returned; if `word` is "outside" of the ecurve, `#UPROC_ECURVE_OOB`
 * will be returned.  In both cases, the objects pointed to by
 * `upper_neighbour` and `upper_class` will be set equal to their `lower_*`
 * counterparts.
 *
 * In case of an inexact match, `#UPROC_ECURVE_INEXACT` will be returned,
 * `lower_*` and their `upper_*` counterparts might or might not differ.
 *
 * \param ecurve            ecurve object
 * \param word              word to search
 * \param lower_neighbour   _OUT_: lower neighbour word
 * \param lower_class       _OUT_: class of the lower neighbour
 * \param upper_neighbour   _OUT_: upper neighbour word
 * \param upper_class       _OUT_: class of the upper neighbour
 *
 * \return `#UPROC_ECURVE_EXACT`, `#UPROC_ECURVE_OOB` or
 * `#UPROC_ECURVE_INEXACT` as described above.
 */
int uproc_ecurve_lookup(const uproc_ecurve *ecurve,
                        const struct uproc_word *word,
                        struct uproc_word *lower_neighbour,
                        uproc_family *lower_class,
                        struct uproc_word *upper_neighbour,
                        uproc_family *upper_class);

uproc_alphabet *uproc_ecurve_alphabet(const uproc_ecurve *ecurve);

uproc_ecurve *uproc_ecurve_loads(enum uproc_ecurve_format format,
                                 uproc_io_stream *stream);

/** Load ecurve from a file
 *
 * Opens a file for reading, allocates a new #uproc_ecurve object and parses
 * the data in the file using the given format.
 *
 * \param ecurve    pointer to ecurve to which the data will be loaded into
 * \param format    format to use, see #uproc_ecurve_format
 * \param iotype    IO type, see #uproc_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPROC_FAILURE  an error occured
 * \retval #UPROC_SUCCESS  else
 */
uproc_ecurve *uproc_ecurve_load(enum uproc_ecurve_format format,
                                enum uproc_io_type iotype,
                                const char *pathfmt, ...);

uproc_ecurve *uproc_ecurve_loadv(enum uproc_ecurve_format format,
                                 enum uproc_io_type iotype,
                                 const char *pathfmt, va_list ap);

int uproc_ecurve_stores(const uproc_ecurve *ecurve,
                        enum uproc_ecurve_format format,
                        uproc_io_stream *stream);

/** Store ecurve to a file
 *
 * Stores an #uproc_ecurve object to a file using the given format.
 *
 * \param ecurve    pointer to ecurve to store
 * \param format    format to use, see #uproc_ecurve_format
 * \param iotype    IO type, see #uproc_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPROC_FAILURE  an error occured
 * \retval #UPROC_SUCCESS  else
 */
int uproc_ecurve_store(const uproc_ecurve *ecurve,
                       enum uproc_ecurve_format format,
                       enum uproc_io_type iotype, const char *pathfmt, ...);

int uproc_ecurve_storev(const uproc_ecurve *ecurve,
                        enum uproc_ecurve_format format,
                        enum uproc_io_type iotype, const char *pathfmt,
                        va_list ap);

/** Map a file to an ecurve
 *
 * The file must have been created by `uproc_ecurve_mmap_store()`, preferably
 * on the same machine.
 *
 * \param ecurve    ecurve to be mapped
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPROC_ESYSCALL     an error occured while calling an OS function
 * \retval #UPROC_EINVAL       malformed file
 * \retval #UPROC_SUCCESS      else
 */
uproc_ecurve *uproc_ecurve_mmap(const char *pathfmt, ...);

uproc_ecurve *uproc_ecurve_mmapv(const char *pathfmt, va_list ap);

/** Release mapping and close the underlying file descriptor
 *
 * \param ecurve    ecurve mapped with `uproc_mmap_map()`
 */
void uproc_ecurve_munmap(uproc_ecurve *ecurve);

/** Store ecurve in a format suitable for `uproc_ecurve_mmap()`
 *
 * \param ecurve    ecurve to store
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 *
 * \retval #UPROC_ESYSCALL     an error occured while calling an OS function
 * \retval #UPROC_SUCCESS      else
 */
int uproc_ecurve_mmap_store(const uproc_ecurve *ecurve,
                            const char *pathfmt, ...);

int uproc_ecurve_mmap_storev(const uproc_ecurve *ecurve, const char *pathfmt,
                             va_list ap);
#endif

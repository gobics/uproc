/* Copyright 2014 Peter Meinicke, Robin Martinjak
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

/** \file uproc/ecurve.h
 *
 * Look up protein sequences in an "Evolutionary Curve"
 *
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
    /** Portable plain text file */
    UPROC_STORAGE_PLAIN,

    /** Machine-dependent binary format */
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


/** Create an empty ecurve object
 *
 * \param alphabet      string to initialize the ecurve's alphabet
 *                      (see uproc_alphabet_init())
 * \param suffix_count  number of entries in the suffix table
 */
uproc_ecurve *uproc_ecurve_create(const char *alphabet, size_t suffix_count);

/** Free memory of an ecurve object
 *
 * \param ecurve    ecurve to destroy
 */
void uproc_ecurve_destroy(uproc_ecurve *ecurve);


/** Append one partial ecurve to another
 *
 * They may not overlap, i.e. the last non-empty prefix of \c dest needs to be
 * less than the first non-empty prefix of \c src.
 *
 * \c dest may not be "mmap()ed"
 *
 * \param dest  ecurve to append to
 * \param src   ecurve to append
 */
int uproc_ecurve_append(uproc_ecurve *dest, const uproc_ecurve *src);


/** Find the closest neighbours of a word in the ecurve
 *
 * NOTE: \c ecurve may not be empty.
 *
 * If \c word was found exactly as it it in the ecurve, ::UPROC_ECURVE_EXACT
 * will be returned; if \c word is "outside" of the ecurve (i.e. its prefix is
 * less than than the smalles or greater than the largest stored non-empty
 * prefix), ::UPROC_ECURVE_OOB will be returned. In both cases, the objects
 * pointed to by \c upper_neighbour and \c upper_class will be set equal to
 * their \c lower_... counterparts.
 *
 * In case of an inexact match, ::UPROC_ECURVE_INEXACT will be returned,
 * \c lower_... and their \c upper_... counterparts will differ.
 *
 * \param ecurve            ecurve object
 * \param word              word to search
 * \param lower_neighbour   _OUT_: lower neighbour word
 * \param lower_class       _OUT_: class of the lower neighbour
 * \param upper_neighbour   _OUT_: upper neighbour word
 * \param upper_class       _OUT_: class of the upper neighbour
 *
 * \return ::UPROC_ECURVE_EXACT, ::UPROC_ECURVE_OOB or
 * ::UPROC_ECURVE_INEXACT as described above.
 */
int uproc_ecurve_lookup(const uproc_ecurve *ecurve,
                        const struct uproc_word *word,
                        struct uproc_word *lower_neighbour,
                        uproc_family *lower_class,
                        struct uproc_word *upper_neighbour,
                        uproc_family *upper_class);


/** Return the internal alphabet */
uproc_alphabet *uproc_ecurve_alphabet(const uproc_ecurve *ecurve);


/** Load ecurve from stream
 *
 * Similar to uproc_ecurve_load(), but reads the data from an already opened
 * file stream.
 */
uproc_ecurve *uproc_ecurve_loads(enum uproc_ecurve_format format,
                                 uproc_io_stream *stream);

/** Load ecurve from file
 *
 * Opens a file for reading, allocates a new ::uproc_ecurve object and parses
 * the data in the file using the given format.
 *
 * If libecurve is compiled on a system that supports mmap(), calling this
 * function with ::UPROC_STORAGE_BINARY as the \c format argument is equivalent
 * to using ::uproc_ecurve_mmap.
 *
 * \param format    format to use, see ::uproc_ecurve_format
 * \param iotype    IO type, see ::uproc_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 */
uproc_ecurve *uproc_ecurve_load(enum uproc_ecurve_format format,
                                enum uproc_io_type iotype,
                                const char *pathfmt, ...);

/** Load ecurve from file
 *
 * Like ::uproc_ecurve_load, but with a \c va_list instead of a variable
 * number of arguments.
 */
uproc_ecurve *uproc_ecurve_loadv(enum uproc_ecurve_format format,
                                 enum uproc_io_type iotype,
                                 const char *pathfmt, va_list ap);


/** Store ecurve to stream
 *
 * Similar to uproc_ecurve_store(), but writes the data to an already opened
 * file stream.
 */
int uproc_ecurve_stores(const uproc_ecurve *ecurve,
                        enum uproc_ecurve_format format,
                        uproc_io_stream *stream);


/** Store ecurve to a file
 *
 * Stores an ::uproc_ecurve object to a file using the given format.
 *
 * If libecurve is compiled on a system that supports mmap(), calling this
 * function with ::UPROC_STORAGE_BINARY as the \c format argument is equivalent
 * to using ::uproc_ecurve_mmap_store.
 *
 * \param ecurve    pointer to ecurve to store
 * \param format    format to use, see ::uproc_ecurve_format
 * \param iotype    IO type, see ::uproc_io_type
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 */
int uproc_ecurve_store(const uproc_ecurve *ecurve,
                       enum uproc_ecurve_format format,
                       enum uproc_io_type iotype, const char *pathfmt, ...);


/** Store ecurve to file
 *
 * Like ::uproc_ecurve_store, but with a \c va_list instead of a variable
 * number of arguments.
 */
int uproc_ecurve_storev(const uproc_ecurve *ecurve,
                        enum uproc_ecurve_format format,
                        enum uproc_io_type iotype, const char *pathfmt,
                        va_list ap);


/** Map a file to an ecurve
 *
 * The file must have been created by `uproc_ecurve_mmap_store()`, preferably
 * on the same machine.
 *
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 */
uproc_ecurve *uproc_ecurve_mmap(const char *pathfmt, ...);


/** Store ecurve to file
 *
 * Like ::uproc_ecurve_store, but with a \c va_list instead of a variable
 * number of arguments.
 */
uproc_ecurve *uproc_ecurve_mmapv(const char *pathfmt, va_list ap);


/** Release mapping and close the underlying file descriptor
 *
 * \param ecurve    ecurve mapped with `uproc_mmap_map()`
 */
void uproc_ecurve_munmap(uproc_ecurve *ecurve);


/** Store ecurve in a format suitable for ::uproc_ecurve_mmap
 *
 * Identical to ::uproc_ecurve_store with ::UPROC_ECURVE
 *
 * \param ecurve    ecurve to store
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 */
int uproc_ecurve_mmap_store(const uproc_ecurve *ecurve,
                            const char *pathfmt, ...);


/** Store ecurve to mmap file
 *
 * Like ::uproc_ecurve_mmap_store, but with a \c va_list instead of a variable
 * number of arguments.
 */
int uproc_ecurve_mmap_storev(const uproc_ecurve *ecurve, const char *pathfmt,
                             va_list ap);
#endif

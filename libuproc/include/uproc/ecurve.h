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
 * Module: \ref grp_datastructs_ecurve
 *
 * \weakgroup grp_datastructs
 * \{
 *
 * \weakgroup grp_datastructs_ecurve
 * \{
 */

#ifndef UPROC_ECURVE_H
#define UPROC_ECURVE_H

#include <stdio.h>
#include <stdarg.h>

#include "uproc/alphabet.h"
#include "uproc/io.h"
#include "uproc/list.h"
#include "uproc/word.h"


/** \defgroup obj_ecurve object uproc_ecurve
 *
 * Evolutionary Curve
 *
 * \{
 */

/** \struct uproc_ecurve
 * \copybrief obj_ecurve
 *
 * See \ref obj_ecurve for details.
 */
typedef struct uproc_ecurve_s uproc_ecurve;


/** Pair of suffix and family
 *
 * uproc_ecurve_add_prefix() expects a \ref grp_datastructs_list  of these as
 * its \c suffixes argument.
 */
struct uproc_ecurve_suffixentry
{
    /** Suffix */
    uproc_suffix suffix;

    /** Protein family */
    uproc_family family;
};


/** Storage format */
enum uproc_ecurve_format
{
    /** Portable plain text file */
    UPROC_ECURVE_PLAIN,

    /** Machine-dependent binary format */
    UPROC_ECURVE_BINARY,
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


/** Create ecurve object
 *
 * \param alphabet      string to initialize the ecurve's alphabet
 *                      (see uproc_alphabet_init())
 * \param suffix_count  number of entries in the suffix table
 */
uproc_ecurve *uproc_ecurve_create(const char *alphabet, size_t suffix_count);


/** Destroy ecurve object */
void uproc_ecurve_destroy(uproc_ecurve *ecurve);


/** Add a prefix entry
 *
 * Adds the prefix \c pfx and the suffix/family pairs in the \c suffixes list
 * to the ecurve.
 *
 * The argument to \c ecurve \em must be an ecurve fulfilling the following
 * conditions:
 * \li created with \c 0 as the second argument to uproc_ecurve_create()
 * \li was extended using this function zero or more times
 * \li not yet finalized with uproc_ecurve_finalize()
 *
 * Because building is done incrementally, the \c pfx argument needs to be
 * greater than it was in the previous call to this function.
 *
 * The \c suffixes argument must be  a \ref grp_datastructs_list with at least
 * one element of type ::uproc_ecurve_suffixentry.
 *
 * \note
 * If this function is used on an ecurve that does not meet the conditions
 * above, the result will be a broken ecurve.
 *
 * Example
 * \code
 * int add_entries(uproc_ecurve *ec, uproc_prefix p,
 *                 uproc_suffix *s, uproc_family *f, int n)
 * {
 *     int i, res;
 *     struct uproc_ecurve_suffixentry e;
 *     uproc_list *list = uproc_list_create(sizeof e);
 *     for (i = 0; i < n; i++) {
 *         e.suffix = s[i];
 *         e.family = f[i];
 *         res = uproc_list_append(list, &e);
 *         if (res) {
 *             // handle error
 *         }
 *     }
 *     res = uproc_ecurve_add_prefix(ec, p, list);
 *     uproc_list_destroy(list);
 *     return res;
 * }
 * \endcode
 */
int uproc_ecurve_add_prefix(uproc_ecurve *ecurve, uproc_prefix pfx,
                            uproc_list *suffixes);


/** Finalize ecurve
 *
 * Finalizes an ecurve that was built using uproc_ecurve_add_prefix().
 * This function \e must be called on a newly-constructed ecurve before it can
 * be used.
 */
int uproc_ecurve_finalize(uproc_ecurve *ecurve);


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


uproc_ecurve *uproc_ecurve_loadps(enum uproc_ecurve_format format,
                                  void (*progress)(double),
                                  uproc_io_stream *stream);

/** Load ecurve from file
 *
 * Opens a file for reading, allocates a new ::uproc_ecurve object and parses
 * the data in the file using the given format.
 *
 * If libecurve is compiled on a system that supports mmap(), calling this
 * function with ::UPROC_ECURVE_BINARY as the \c format argument is equivalent
 * to using uproc_ecurve_mmap().
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
 * Like uproc_ecurve_load(), but periodically calls \c progress to report the
 * current progress (in percent).
 */
uproc_ecurve *uproc_ecurve_loadp(enum uproc_ecurve_format format,
                                 enum uproc_io_type iotype,
                                 void (*progress)(double),
                                 const char *pathfmt, ...);


/** Load ecurve from file
 *
 * Like uproc_ecurve_load(), but with a \c va_list instead of a variable
 * number of arguments.
 */
uproc_ecurve *uproc_ecurve_loadv(enum uproc_ecurve_format format,
                                 enum uproc_io_type iotype,
                                 const char *pathfmt, va_list ap);


/** Load ecurve from file
 *
 * Like uproc_ecurve_loadp(), but with a \c va_list instead of a variable
 * number of arguments.
 */
uproc_ecurve *uproc_ecurve_loadpv(enum uproc_ecurve_format format,
                                  enum uproc_io_type iotype,
                                  void (*progress)(double),
                                  const char *pathfmt, va_list ap);


/** Store ecurve to stream
 *
 * Similar to uproc_ecurve_store(), but writes the data to an already opened
 * file stream.
 */
int uproc_ecurve_stores(const uproc_ecurve *ecurve,
                        enum uproc_ecurve_format format,
                        uproc_io_stream *stream);


/** Store ecurve to stream
 *
 * Similar to uproc_ecurve_storep(), but writes the data to an already opened
 * file stream.
 */
int uproc_ecurve_storeps(const uproc_ecurve *ecurve,
                         enum uproc_ecurve_format format,
                         void (*progress)(double),
                         uproc_io_stream *stream);


/** Store ecurve to file
 *
 * Stores a ::uproc_ecurve object to a file using the given format.
 *
 * If libecurve is compiled on a system that supports mmap(), calling this
 * function with ::UPROC_ECURVE_BINARY as the \c format argument is equivalent
 * to using uproc_ecurve_mmap_store().
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
 * Like uproc_ecurve_store(), but periodically calls \c progress to report the
 * current progress (in percent).
 */
int uproc_ecurve_storep(const uproc_ecurve *ecurve,
                       enum uproc_ecurve_format format,
                       enum uproc_io_type iotype,
                       void (*progress)(double),
                       const char *pathfmt, ...);


/** Store ecurve to file
 *
 * Like uproc_ecurve_store(), but with a \c va_list instead of a variable
 * number of arguments.
 */
int uproc_ecurve_storev(const uproc_ecurve *ecurve,
                        enum uproc_ecurve_format format,
                        enum uproc_io_type iotype, const char *pathfmt,
                        va_list ap);


/** Store ecurve to file
 *
 * Like uproc_ecurve_storep(), but with a \c va_list instead of a variable
 * number of arguments.
 */
int uproc_ecurve_storepv(const uproc_ecurve *ecurve,
                         enum uproc_ecurve_format format,
                         enum uproc_io_type iotype,
                         void (*progress)(double),
                         const char *pathfmt,
                         va_list ap);


/** Map a file to an ecurve
 *
 * The file must have been created by uproc_ecurve_mmap_store(), preferably
 * on the same machine.
 *
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 */
uproc_ecurve *uproc_ecurve_mmap(const char *pathfmt, ...);


/** Store ecurve to file
 *
 * Like uproc_ecurve_store(), but with a \c va_list instead of a variable
 * number of arguments.
 */
uproc_ecurve *uproc_ecurve_mmapv(const char *pathfmt, va_list ap);


/** Release mapping and close the underlying file descriptor
 *
 * \param ecurve    ecurve mapped with uproc_mmap_map()
 */
void uproc_ecurve_munmap(uproc_ecurve *ecurve);


/** Store ecurve in a format suitable for uproc_ecurve_mmap()
 *
 * Identical to uproc_ecurve_store() with ::UPROC_ECURVE_BINARY
 *
 * \param ecurve    ecurve to store
 * \param pathfmt   printf format string for file path
 * \param ...       format string arguments
 */
int uproc_ecurve_mmap_store(const uproc_ecurve *ecurve,
                            const char *pathfmt, ...);


/** Store ecurve to mmap file
 *
 * Like uproc_ecurve_mmap_store(), but with a \c va_list instead of a variable
 * number of arguments.
 */
int uproc_ecurve_mmap_storev(const uproc_ecurve *ecurve, const char *pathfmt,
                             va_list ap);
/** \} */

/**
 * \}
 * \}
 */
#endif

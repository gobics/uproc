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

/** \file uproc/io.h
 *
 * Module: \ref grp_io_io
 *
 * \weakgroup grp_io
 * \{
 * \weakgroup grp_io_io
 *
 * \details
 * Wrappers for transparent access to different types of IO streams
 *
 * \{
 */

#ifndef UPROC_IO_H
#define UPROC_IO_H

#include <stdio.h>
#include <stdarg.h>

/** \defgroup obj_io_stream object uproc_io_stream
 *
 * Optionally compressed I/O stream
 *
 * Unless noted otherwise, a function called \c uproc_io_NAME() corresponds
 * directly to its \c stdio.h counterpart called \c fNAME(). Most of the time,
 * there also exists a similar \c gzNAME() function in \c zlib, but the order
 * and types of parameters are always adapted from \c stdio.h.
 *
 * \{
 */

/** \struct uproc_io_stream
 * \copybrief obj_io_stream
 *
 * See \ref obj_io_stream for details.
 */
typedef struct uproc_io_stream uproc_io_stream;

/** Underlying stream type */
enum uproc_io_type {
    /** standard C's FILE pointer */
    UPROC_IO_STDIO,
    /** transparent gzip stream using zlib */
    UPROC_IO_GZIP,
};

/** Third argument to uproc_io_seek()
 *
 * These correspond to SEEK_SET and SEEK_CUR of stdio.h. SEEK_END is missing
 * because gz streams don't support it.
 */
enum uproc_io_seek_whence {
    /** Set the cursor to an absolute position */
    UPROC_IO_SEEK_SET,

    /** Set the new position relative to the current one */
    UPROC_IO_SEEK_CUR,
};

/** Open a file
 *
 * Corresponds to using
 *
 * \li \c fopen() if \c type is ::UPROC_IO_STDIO
 * \li \c gzopen() if \c type is ::UPROC_IO_GZIP (and libuproc was compiled
 *     with zlib support)
 *
 *
 * where the file name is constructed by formatting \c pathfmt with the
 * subsequent arguments (using \c sprintf).
 */
uproc_io_stream *uproc_io_open(const char *mode, enum uproc_io_type type,
                               const char *pathfmt, ...);

/** Open a file using a va_list
 *
 * Just like ::uproc_io_open, but with a \c va_list instead of a variable
 * number of arguments.
 */
uproc_io_stream *uproc_io_openv(const char *mode, enum uproc_io_type type,
                                const char *pathfmt, va_list ap);

/** Close a file stream */
int uproc_io_close(uproc_io_stream *stream);

/** Formatted output */
int uproc_io_printf(uproc_io_stream *stream, const char *fmt, ...);

/** Read binary data */
size_t uproc_io_read(void *ptr, size_t size, size_t nmemb,
                     uproc_io_stream *stream);

/** Write binary data */
size_t uproc_io_write(const void *ptr, size_t size, size_t nmemb,
                      uproc_io_stream *stream);

/** (Partially) read a line
 *
 * Corresponds to \c fgets(), \em not \c gets()!
 */
char *uproc_io_gets(char *s, int size, uproc_io_stream *stream);

/** Read an entire line
 *
 * Reads a whole line into \c *lineptr, reallocating it if necessary
 * (works exactly like the POSIX \c getline function).
 */
long uproc_io_getline(char **lineptr, size_t *n, uproc_io_stream *stream);

/** Write a character */
int uproc_io_putc(int c, uproc_io_stream *stream);

/** Write a string and a trailing newline */
int uproc_io_puts(const char *s, uproc_io_stream *stream);

/** Set the file position */
int uproc_io_seek(uproc_io_stream *stream, long offset,
                  enum uproc_io_seek_whence whence);

/** Obtain current file position */
long uproc_io_tell(uproc_io_stream *stream);

/** Test end-of-file indicator */
int uproc_io_eof(uproc_io_stream *stream);
/** \} obj_io_stream */

/** \defgroup grp_io_io_stdstream Wrapped standard IO streams
 * \{
 */

/** Wrap standard input streams
 *
 * Called by the \ref grp_io_io_stdstream macros.
 */
uproc_io_stream *uproc_io_stdstream(FILE *stream);

/** stdin, possibly gzip compressed
 *
 */
#define uproc_stdin uproc_io_stdstream_gz(stdin)

/** stdout, uncompressed */
#define uproc_stdout uproc_io_stdstream(stdout)

/** stderr, uncompressed */
#define uproc_stderr uproc_io_stdstream(stderr)
/** \} grp_io_io_stdstream */

/** \defgroup grp_io_io_stdstream_gz Wrapped IO streams with gz compression
 *
 * \note
 * These macros are only available if libuproc was compiled with zlib support.
 * \c uproc_stdin_gz is not needed, zlib is used by default and can also read
 * uncompressed data from \c stdin.
 *
 * \note
 * Using one of these macros registers an exit handler using \c atexit().
 *
 * \{
 */

/** Wraps gz input streams
 *
 * Called by the \ref grp_io_io_stdstream_gz macros.
 */
uproc_io_stream *uproc_io_stdstream_gz(FILE *stream);

/** stdout, gzip compressed */
#define uproc_stdout_gz uproc_io_stdstream_gz(stdout)

/** stderr, gzip compressed */
#define uproc_stderr_gz uproc_io_stdstream_gz(stderr)
/** \} grp_io_io_stdstream_gz */

/**
 * \} grp_io_io
 * \} grp_io
 */
#endif

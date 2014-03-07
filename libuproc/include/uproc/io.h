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
 * Wrappers for accessing different types IO streams
 *
 * \weakgroup grp_io
 * \{
 * \weakgroup grp_io_io
 * \{
 */

#ifndef UPROC_IO_H
#define UPROC_IO_H

#include <stdio.h>
#include <stdarg.h>


/** \defgroup obj_io_stream object uproc_io_stream
 * \{
 */

/** Opaque type for IO streams
 *
 */
typedef struct uproc_io_stream uproc_io_stream;


/** Underlying stream type */
enum uproc_io_type
{
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
enum uproc_io_seek_whence
{
    /** Set the cursor to an absolute position */
    UPROC_IO_SEEK_SET,

    /** Set the new position relative to the current one */
    UPROC_IO_SEEK_CUR,
};

/** Open a file
 *
 * Opens the file whose name is constructed by formatting \c pathfmt with the
 * subsequent arguments (using \c sprintf).
 */
uproc_io_stream *uproc_io_open(const char *mode, enum uproc_io_type type,
                               const char *pathfmt, ...);


/** Open a file using a va_list
 *
 * Justl ike ::uproc_io_open, but with a \c va_list instead of a variable
 * number of arguments.
 */
uproc_io_stream *uproc_io_openv(const char *mode, enum uproc_io_type type,
                                const char *pathfmt, va_list ap);

int uproc_io_close(uproc_io_stream *stream);

int uproc_io_printf(uproc_io_stream *stream, const char *fmt, ...);

size_t uproc_io_read(void *ptr, size_t size, size_t nmemb,
                     uproc_io_stream *stream);

size_t uproc_io_write(const void *ptr, size_t size, size_t nmemb,
                      uproc_io_stream *stream);

char *uproc_io_gets(char *s, int size, uproc_io_stream *stream);

long uproc_io_getline(char **lineptr, size_t *n, uproc_io_stream *stream);

int uproc_io_seek(uproc_io_stream *stream, long offset,
                  enum uproc_io_seek_whence whence);

long uproc_io_tell(uproc_io_stream *stream);

int uproc_io_eof(uproc_io_stream *stream);
/** \} */


/** \defgroup grp_io_io_stdstream Wrapped standard IO streams
 * \{
 */

/** Wrap standard input streams
 *
 * Called by the \ref grp_io_io_stdstream macros.
 */
uproc_io_stream *uproc_io_stdstream(FILE *stream);


/** stdin, possibly gzip compressed */
#define uproc_stdin uproc_io_stdstream_gz(stdin)


/** stdout, uncompressed */
#define uproc_stdout uproc_io_stdstream(stdout)


/** stderr, uncompressed */
#define uproc_stderr uproc_io_stdstream(stderr)
/** \} */


/** \defgroup grp_io_io_stdstream_gz Wrapped IO streams with gz compression
 *
 * These macros are only available if libuproc was compiled with zlib support.
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
/** \} */


/**
 * \}
 * \}
 */
#endif

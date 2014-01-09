/** \file uproc/io.h
 *
 * Wrappers for accessing different IO streams
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

#ifndef UPROC_IO_H
#define UPROC_IO_H


#include <stdio.h>
#include <stdarg.h>

/** Underlying stream type */
enum uproc_io_type
{
    /** standard C's FILE pointer */
    UPROC_IO_STDIO,
#if HAVE_ZLIB_H
    /** transparent gzip stream using zlib */
    UPROC_IO_GZIP,
#else
    /** or fall back to stdio */
    UPROC_IO_GZIP = UPROC_IO_STDIO,
#endif
};


typedef struct uproc_io_stream uproc_io_stream;

uproc_io_stream *uproc_io_stdstream(FILE *stream);
#define uproc_stdin uproc_io_stdstream(stdin)
#define uproc_stdout uproc_io_stdstream(stdout)
#define uproc_stderr uproc_io_stdstream(stderr)

#if HAVE_ZLIB_H
uproc_io_stream *uproc_io_stdstream_gz(FILE *stream);
#define uproc_stdin_gz uproc_io_stdstream_gz(stdin)
#define uproc_stdout_gz uproc_io_stdstream_gz(stdout)
#define uproc_stderr_gz uproc_io_stdstream_gz(stderr)
#undef uproc_stdin
#define uproc_stdin uproc_stdin_gz
#endif

uproc_io_stream *uproc_io_open(const char *mode, enum uproc_io_type type,
                               const char *pathfmt, ...);

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

int uproc_io_seek(uproc_io_stream *stream, long offset, int whence);

int uproc_io_eof(uproc_io_stream *stream);
#endif

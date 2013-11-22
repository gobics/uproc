#ifndef UPRO_IO_H
#define UPRO_IO_H

/** \file upro/io.h
 *
 * Wrappers for accessing different IO streams
 *
 */

#include <stdio.h>
#include <stdarg.h>

/** Underlying stream type */
enum upro_io_type
{
    /** standard C's FILE pointer */
    UPRO_IO_STDIO,
#if HAVE_ZLIB_H
    /** transparent gzip stream using zlib */
    UPRO_IO_GZIP,
#else
    /** or fall back to stdio */
    UPRO_IO_GZIP = UPRO_IO_STDIO,
#endif
};


typedef struct upro_io_stream upro_io_stream;

upro_io_stream *upro_io_stdstream(FILE *stream);
#define upro_stdin upro_io_stdstream(stdin)
#define upro_stdout upro_io_stdstream(stdout)
#define upro_stderr upro_io_stdstream(stderr)

#if HAVE_ZLIB_H
upro_io_stream *upro_io_stdstream_gz(FILE *stream);
#define upro_stdin_gz upro_io_stdstream_gz(stdin)
#define upro_stdout_gz upro_io_stdstream_gz(stdout)
#define upro_stderr_gz upro_io_stdstream_gz(stderr)
#undef upro_stdin
#define upro_stdin upro_stdin_gz
#endif

upro_io_stream *upro_io_open(const char *mode, enum upro_io_type type,
        const char *format, ...);

upro_io_stream *upro_io_openv(const char *mode, enum upro_io_type type,
        const char *format, va_list ap);

int upro_io_close(upro_io_stream *stream);

int upro_io_printf(upro_io_stream *stream, const char *fmt, ...);

size_t upro_io_read(void *ptr, size_t size, size_t nmemb, upro_io_stream *stream);

size_t upro_io_write(const void *ptr, size_t size, size_t nmemb,
        upro_io_stream *stream);

char *upro_io_gets(char *s, int size, upro_io_stream *stream);

long upro_io_getline(char **lineptr, size_t *n, upro_io_stream *stream);

int upro_io_seek(upro_io_stream *stream, long offset, int whence);

#endif

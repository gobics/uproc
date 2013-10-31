#ifndef EC_IO_H
#define EC_IO_H

/** \file ecurve/io.h
 *
 * Wrappers for accessing different IO streams
 *
 */

#include <stdio.h>

/** Underlying stream type */
enum ec_io_type
{
    /** standard C's FILE pointer */
    EC_IO_STDIO,
#if HAVE_ZLIB
    /** transparent gzip stream using zlib */
    EC_IO_GZIP,
#else
    /** or fall back to stdio */
    EC_IO_GZIP = EC_IO_STDIO,
#endif
};


typedef struct ec_io_stream ec_io_stream;

ec_io_stream *ec_io_stdstream(FILE *stream);
#define ec_stdin ec_io_stdstream(stdin)
#define ec_stdout ec_io_stdstream(stdout)
#define ec_stderr ec_io_stdstream(stderr)

#if HAVE_FILENO && HAVE_ZLIB
ec_io_stream *ec_io_stdstream_gz(FILE *stream);
#define ec_stdin_gz ec_io_stdstream_gz(stdin)
#define ec_stdout_gz ec_io_stdstream_gz(stdout)
#define ec_stderr_gz ec_io_stdstream_gz(stderr)
#undef ec_stdin
#define ec_stdin ec_stdin_gz
#endif

ec_io_stream *ec_io_open(const char *path, const char *mode,
        enum ec_io_type type);

int ec_io_close(ec_io_stream *stream);

int ec_io_printf(ec_io_stream *stream, const char *fmt, ...);

size_t ec_io_read(void *ptr, size_t size, size_t nmemb, ec_io_stream *stream);

size_t ec_io_write(const void *ptr, size_t size, size_t nmemb,
        ec_io_stream *stream);

char *ec_io_gets(char *s, int size, ec_io_stream *stream);

long ec_io_getline(char **lineptr, size_t *n, ec_io_stream *stream);

int ec_io_seek(ec_io_stream *stream, long offset, int whence);

#endif

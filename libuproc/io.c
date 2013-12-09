/* Wrappers for accessing different IO streams
 *
 * Copyright 2013 Peter Meinicke, Robin Martinjak
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>

#if HAVE_ZLIB_H
#include <zlib.h>
#endif

#include "uproc/common.h"
#include "uproc/error.h"
#include "uproc/io.h"

#define GZIP_BUFSZ (512 * (1 << 10))

struct uproc_io_stream
{
    enum uproc_io_type type;
    union {
        FILE *fp;
#if HAVE_ZLIB_H
        gzFile gz;
#endif
    } s;
    bool stdstream;
};

uproc_io_stream *
uproc_io_stdstream(FILE *stream)
{
    static uproc_io_stream s[3];
    size_t i;
    if (stream == stdin) {
        i = 0;
    }
    else if (stream == stdout) {
        i = 1;
    }
    else {
        i = 2;
    }
    if (!s[i].s.fp) {
        s[i].type = UPROC_IO_STDIO;
        s[i].s.fp = stream;
        s[i].stdstream = true;
    }
    return &s[i];
}

#if HAVE_ZLIB_H
static void
close_stdstream_gz(void)
{
    (void) uproc_io_stdstream_gz(NULL);
}

uproc_io_stream *
uproc_io_stdstream_gz(FILE *stream)
{
    static uproc_io_stream s[3];
    static char *mode[] = { "r", "w", "w" };
    size_t i;
    if (s[0].type != UPROC_IO_GZIP) {
        for (i = 0; i < 3; i++) {
            s[i].type = UPROC_IO_GZIP;
            s[i].s.gz = NULL;
            s[i].stdstream = true;
        }
        atexit(close_stdstream_gz);
    }

    /* called by close_stdstream_gz */
    if (!stream) {
        for (i = 0; i < 3; i++) {
            if (s[i].s.gz) {
                gzclose(s[i].s.gz);
            }
        }
        return NULL;
    }

    if (stream == stdin) {
        i = 0;
    }
    else if (stream == stdout) {
        i = 1;
    }
    else {
        i = 2;
    }
    if (!s[i].s.gz) {
        s[i].s.gz = gzdopen(i, mode[i]);
    }
    return &s[i];
}

#if ZLIB_VERNUM < 0x1235
#define gzbuffer(file, size) 0
#endif

#if ZLIB_VERNUM < 0x1271
static int
gzvprintf(gzFile file, const char *format, va_list va)
{
    char *buf;
    size_t n;
    va_list copy;
    va_copy(copy, va);

    n = vsnprintf(NULL, 0, format, copy);
    va_end(copy);

    buf = malloc(n + 1);
    if (!buf) {
        return -1;
    }
    vsnprintf(buf, n + 1, format, va);
    n = gzwrite(file, buf, n);
    free(buf);
    return n;
}
#endif
#endif

static uproc_io_stream *
io_open(const char *path, const char *mode, enum uproc_io_type type)
{
    uproc_io_stream *stream = malloc(sizeof *stream);
    if (!stream) {
        uproc_error(UPROC_ENOMEM);
        return NULL;
    }
    stream->type = type;
    stream->stdstream = false;
    switch (stream->type) {
        case UPROC_IO_STDIO:
            if (!(stream->s.fp = fopen(path, mode))) {
                goto error_errno;
            }
            break;
#if HAVE_ZLIB_H
        case UPROC_IO_GZIP:
            if (!(stream->s.gz = gzopen(path, mode))) {
                /* gzopen sets errno to 0 if memory allocation failed */
                if (!errno) {
                    uproc_error(UPROC_ENOMEM);
                    goto error;
                }
                goto error_errno;
            }
            (void) gzbuffer(stream->s.gz, GZIP_BUFSZ);
            break;
#endif
        default:
            goto error;
    }
    return stream;
error_errno:
    uproc_error_msg(UPROC_ERRNO, "can't open \"%s\" with mode \"%s\"",
                    path, mode);
error:
    free(stream);
    return NULL;
}

uproc_io_stream *
uproc_io_open(const char *mode, enum uproc_io_type type, const char *pathfmt,
              ...)
{
    uproc_io_stream *stream;
    va_list ap;
    va_start(ap, pathfmt);
    stream = uproc_io_openv(mode, type, pathfmt, ap);
    va_end(ap);
    return stream;
}

uproc_io_stream *
uproc_io_openv(const char *mode, enum uproc_io_type type, const char *pathfmt,
               va_list ap)
{
    uproc_io_stream *stream;
    char *buf;
    size_t n;
    va_list aq;

    va_copy(aq, ap);
    n = vsnprintf(NULL, 0, pathfmt, aq);
    va_end(aq);

    buf = malloc(n + 1);
    if (!buf) {
        return NULL;
    }
    vsprintf(buf, pathfmt, ap);

    stream = io_open(buf, mode, type);
    free(buf);
    return stream;
}

int
uproc_io_close(uproc_io_stream *stream)
{
    int res = 1;
    switch (stream->type) {
        case UPROC_IO_STDIO:
            if (stream->s.fp) {
                res = fclose(stream->s.fp);
                if (res) {
                    res = -1;
                }
                stream->s.fp = NULL;
            }
            break;
#if HAVE_ZLIB_H
        case UPROC_IO_GZIP:
            if (stream->s.gz) {
                res = gzclose(stream->s.gz);
                stream->s.gz = NULL;
                res = res == Z_OK ? 0 : -1;
                if (res == Z_OK) {
                    res = 0;
                }
                else {
                    res = uproc_error_msg(UPROC_ERRNO,
                                         "error closing gz stream");
                }
            }
            break;
#endif
        default:
            return uproc_error_msg(UPROC_EINVAL, "invalid stream");
    }
    if (!stream->stdstream) {
        free(stream);
    }
    return res;
}

int
uproc_io_printf(uproc_io_stream *stream, const char *fmt, ...)
{
    int res = -1;
    va_list ap;
    va_start(ap, fmt);
    switch (stream->type) {
        case UPROC_IO_STDIO:
            res = vfprintf(stream->s.fp, fmt, ap);
            break;
#if HAVE_ZLIB_H
        case UPROC_IO_GZIP:
            res = gzvprintf(stream->s.gz, fmt, ap);
            break;
#endif
        default:
            res = uproc_error_msg(UPROC_EINVAL, "invalid stream");
    }
    va_end(ap);
    return res;
}

size_t
uproc_io_read(void *ptr, size_t size, size_t nmemb, uproc_io_stream *stream)
{
    switch (stream->type) {
        case UPROC_IO_STDIO:
            return fread(ptr, size, nmemb, stream->s.fp);
#if HAVE_ZLIB_H
        case UPROC_IO_GZIP:
            {
                size_t i, n;
                for (i = 0; i < nmemb; i++) {
                    n = gzread(stream->s.gz, (char*)ptr + i * size, size);
                    if (n != size) {
                        break;
                    }
                }
                return i;
            }
#endif
    }
    uproc_error_msg(UPROC_EINVAL, "invalid stream");
    return 0;
}

size_t
uproc_io_write(const void *ptr, size_t size, size_t nmemb,
               uproc_io_stream *stream)
{
    switch (stream->type) {
        case UPROC_IO_STDIO:
            return fwrite(ptr, size, nmemb, stream->s.fp);
#if HAVE_ZLIB_H
        case UPROC_IO_GZIP:
            {
                size_t i, n;
                for (i = 0; i < nmemb; i++) {
                    n = gzwrite(stream->s.gz, (char*)ptr + i * size, size);
                    if (n != size) {
                        break;
                    }
                }
                return i;
            }
#endif
    }
    uproc_error_msg(UPROC_EINVAL, "invalid stream");
    return 0;
}

char *
uproc_io_gets(char *s, int size, uproc_io_stream *stream)
{
    char *res;
    switch (stream->type) {
        case UPROC_IO_STDIO:
            res = fgets(s, size, stream->s.fp);
            if (!res) {
                uproc_error_msg(UPROC_EIO, "failed to read from stream");
            }
            break;
#if HAVE_ZLIB_H
        case UPROC_IO_GZIP:
            res = gzgets(stream->s.gz, s, size);
            if (!res) {
                uproc_error_msg(UPROC_EIO, "failed to read from gz stream");
            }
            break;
#endif
        default:
            uproc_error_msg(UPROC_EINVAL, "invalid stream");
            return NULL;
    }
    return res;
}

long
uproc_io_getline(char **lineptr, size_t *n, uproc_io_stream *stream)
{
#if defined(_GNU_SOURCE) || _POSIX_C_SOURCE >= 200809L || _XOPEN_SOURCE >= 700
    if (stream->type == UPROC_IO_STDIO) {
        ssize_t res = getline(lineptr, n, stream->s.fp);
        if (res == -1) {
            if (feof(stream->s.fp)) {
                uproc_error(UPROC_SUCCESS);
            }
            else {
                uproc_error(UPROC_ERRNO);
            }
        }
        return res;
    }
#endif

    char buf[4097];
    size_t len, total = 0;
    do {
        if (!uproc_io_gets(buf, sizeof buf, stream)) {
            if (!total) {
                uproc_error(UPROC_SUCCESS);
                return -1;
            }
            break;
        }
        len = strlen(buf);
        if (!*lineptr || *n < total + len) {
            void *tmp = realloc(*lineptr, total + len);
            if (!tmp) {
                uproc_error(UPROC_ENOMEM);
                return -1;
            }
            *lineptr = tmp;
            *n = total + len;
        }
        memcpy(*lineptr + total, buf, len);
        total += len;
    } while (buf[len - 1] != '\n');
    return total;
}

int
uproc_io_seek(uproc_io_stream *stream, long offset, int whence)
{
    switch (stream->type) {
        case UPROC_IO_STDIO:
            return fseek(stream->s.fp, offset, whence);
#if HAVE_ZLIB_H
        case UPROC_IO_GZIP:
            return gzseek(stream->s.gz, offset, whence) >= 0 ? 0 : -1;
#endif
    }
    uproc_error_msg(UPROC_EINVAL, "invalid stream");
    return -1;
}

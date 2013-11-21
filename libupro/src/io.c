#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>

#if HAVE_ZLIB
#include <zlib.h>
#endif

#include "upro/common.h"
#include "upro/error.h"
#include "upro/io.h"

#define GZIP_BUFSZ (512 * (1 << 10))

struct upro_io_stream
{
    enum upro_io_type type;
    union {
        FILE *fp;
#if HAVE_ZLIB
        gzFile gz;
#endif
    } s;
    bool stdstream;
};

upro_io_stream *
upro_io_stdstream(FILE *stream)
{
    static upro_io_stream s[3];
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
        s[i].type = UPRO_IO_STDIO;
        s[i].s.fp = stream;
        s[i].stdstream = true;
    }
    return &s[i];
}

#if HAVE_ZLIB
static void
close_stdstream_gz(void)
{
    (void) upro_io_stdstream_gz(NULL);
}

upro_io_stream *
upro_io_stdstream_gz(FILE *stream)
{
    static upro_io_stream s[3];
    static char *mode[] = { "r", "w", "w" };
    size_t i;
    if (s[0].type != UPRO_IO_GZIP) {
        for (i = 0; i < 3; i++) {
            s[i].type = UPRO_IO_GZIP;
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
#endif

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

static upro_io_stream *
io_open(const char *path, const char *mode, enum upro_io_type type)
{
    upro_io_stream *stream = malloc(sizeof *stream);
    if (!stream) {
        return NULL;
    }
    stream->type = type;
    stream->stdstream = false;
    switch (stream->type) {
        case UPRO_IO_STDIO:
            if (!(stream->s.fp = fopen(path, mode))) {
                upro_error(UPRO_ERRNO);
                goto error;
            }
            break;
#if HAVE_ZLIB
        case UPRO_IO_GZIP:
            if (!(stream->s.gz = gzopen(path, mode))) {
                /* gzopen sets errno to 0 if memory allocation failed */
                upro_error(errno ? UPRO_ERRNO : UPRO_ENOMEM);
                goto error;
            }
            (void) gzbuffer(stream->s.gz, GZIP_BUFSZ);
            break;
#endif
        default:
            goto error;
    }
    return stream;
error:
    free(stream);
    return NULL;
}

upro_io_stream *
upro_io_open(const char *mode, enum upro_io_type type,
        const char *pathfmt, ...)
{
    upro_io_stream *stream;
    va_list ap;
    va_start(ap, pathfmt);
    stream = upro_io_openv(mode, type, pathfmt, ap);
    va_end(ap);
    return stream;
}

upro_io_stream *
upro_io_openv(const char *mode, enum upro_io_type type,
        const char *pathfmt, va_list ap)
{
    upro_io_stream *stream;
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
upro_io_close(upro_io_stream *stream)
{
    int res = 1;
    switch (stream->type) {
        case UPRO_IO_STDIO:
            if (stream->s.fp) {
                res = fclose(stream->s.fp);
                if (res) {
                    res = -1;
                }
                stream->s.fp = NULL;
            }
            break;
#if HAVE_ZLIB
        case UPRO_IO_GZIP:
            if (stream->s.gz) {
                res = gzclose(stream->s.gz);
                stream->s.gz = NULL;
                res = res == Z_OK ? 0 : -1;
                if (res == Z_OK) {
                    res = 0;
                }
                else {
                    res = upro_error_msg(UPRO_ERRNO,
                                         "error closing gz stream");
                }
            }
            break;
#endif
        default:
            return upro_error_msg(UPRO_EINVAL, "invalid stream");
    }
    if (!stream->stdstream) {
        free(stream);
    }
    return res;
}

int
upro_io_printf(upro_io_stream *stream, const char *fmt, ...)
{
    int res = -1;
    va_list ap;
    va_start(ap, fmt);
    switch (stream->type) {
        case UPRO_IO_STDIO:
            res = vfprintf(stream->s.fp, fmt, ap);
            break;
#if HAVE_ZLIB
        case UPRO_IO_GZIP:
            res = gzvprintf(stream->s.gz, fmt, ap);
            break;
#endif
        default:
            res = upro_error_msg(UPRO_EINVAL, "invalid stream");
    }
    va_end(ap);
    return res;
}

size_t
upro_io_read(void *ptr, size_t size, size_t nmemb, upro_io_stream *stream)
{
    switch (stream->type) {
        case UPRO_IO_STDIO:
            return fread(ptr, size, nmemb, stream->s.fp);
#if HAVE_ZLIB
        case UPRO_IO_GZIP:
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
    upro_error_msg(UPRO_EINVAL, "invalid stream");
    return 0;
}

size_t
upro_io_write(const void *ptr, size_t size, size_t nmemb, upro_io_stream *stream)
{
    switch (stream->type) {
        case UPRO_IO_STDIO:
            return fwrite(ptr, size, nmemb, stream->s.fp);
#if HAVE_ZLIB
        case UPRO_IO_GZIP:
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
    upro_error_msg(UPRO_EINVAL, "invalid stream");
    return 0;
}

char *
upro_io_gets(char *s, int size, upro_io_stream *stream)
{
    char *res;
    switch (stream->type) {
        case UPRO_IO_STDIO:
            res = fgets(s, size, stream->s.fp);
            if (!res) {
                upro_error_msg(UPRO_EIO, "failed to read from stream");
            }
            break;
#if HAVE_ZLIB
        case UPRO_IO_GZIP:
            res = gzgets(stream->s.gz, s, size);
            if (!res) {
                upro_error_msg(UPRO_EIO, "failed to read from gz stream");
            }
            break;
#endif
        default:
            upro_error_msg(UPRO_EINVAL, "invalid stream");
            return NULL;
    }
    return res;
}

long
upro_io_getline(char **lineptr, size_t *n, upro_io_stream *stream)
{
#if defined(_GNU_SOURCE) || _POSIX_C_SOURCE >= 200809L || _XOPEN_SOURCE >= 700
    if (stream->type == UPRO_IO_STDIO) {
        ssize_t res = getline(lineptr, n, stream->s.fp);
        if (res == -1) {
            if (feof(stream->s.fp)) {
                upro_error(UPRO_SUCCESS);
            }
            else {
                upro_error(UPRO_ERRNO);
            }
        }
        return res;
    }
#endif

    char buf[4097];
    size_t len, total = 0;
    do {
        if (!upro_io_gets(buf, sizeof buf, stream)) {
            if (!total) {
                upro_error(UPRO_SUCCESS);
                return -1;
            }
            break;
        }
        len = strlen(buf);
        if (!*lineptr || *n < total + len) {
            void *tmp = realloc(*lineptr, total + len);
            if (!tmp) {
                upro_error(UPRO_ENOMEM);
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
upro_io_seek(upro_io_stream *stream, long offset, int whence)
{
    switch (stream->type) {
        case UPRO_IO_STDIO:
            return fseek(stream->s.fp, offset, whence);
#if HAVE_ZLIB
        case UPRO_IO_GZIP:
            return gzseek(stream->s.gz, offset, whence) >= 0 ? 0 : -1;
#endif
    }
    upro_error_msg(UPRO_EINVAL, "invalid stream");
    return -1;
}

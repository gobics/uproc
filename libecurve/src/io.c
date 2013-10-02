#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#if HAVE_ZLIB
#include <zlib.h>
#endif

#include "ecurve/common.h"
#include "ecurve/io.h"

#define GZIP_BUFSZ (512 * (1 << 10))

struct ec_io_stream
{
    enum ec_io_type type;
    union {
        FILE *fp;
#if HAVE_ZLIB
        gzFile gz;
#endif
    } s;
    bool stdstream;
};

ec_io_stream *
ec_io_stdstream(FILE *stream)
{
    static ec_io_stream s[3];
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
        s[i].type = EC_IO_STDIO;
        s[i].stdstream = true;
        s[i].s.fp = stream;
    }
    return &s[i];
}

#if HAVE_FILENO && HAVE_ZLIB
static void
close_stdstream_gz(void)
{
    (void) ec_io_stdstream_gz(NULL);
}

ec_io_stream *
ec_io_stdstream_gz(FILE *stream)
{
    static ec_io_stream s[3];
    static char *mode[] = { "r", "w", "w" };
    size_t i;
    if (!stream) {
        for (i = 0; i < 3; i++) {
            if (s[i].s.gz) {
                gzclose(s[i].s.gz);
                s[i].s.gz = NULL;
            }
        }
        return NULL;
    }

    if (s[0].type != EC_IO_GZIP) {
        for (i = 0; i < 3; i++) {
            s[i].type = EC_IO_GZIP;
            s[i].stdstream = true;
            s[i].s.gz = gzdopen(i, mode[i]);
        }
        atexit(close_stdstream_gz);
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
    return &s[i];
}
#endif


ec_io_stream *
ec_io_open(const char *path, const char *mode, enum ec_io_type type)
{
    ec_io_stream *stream = malloc(sizeof *stream);
    if (!stream) {
        return NULL;
    }
    stream->type = type;
    switch (stream->type) {
        case EC_IO_STDIO:
            if (!(stream->s.fp = fopen(path, mode))) {
                goto error;
            }
            break;
#if HAVE_ZLIB
        case EC_IO_GZIP:
            if (!(stream->s.gz = gzopen(path, mode))) {
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

int
ec_io_close(ec_io_stream *stream)
{
    int res = 1;
    switch (stream->type) {
        case EC_IO_STDIO:
            if (stream->s.fp) {
                res = fclose(stream->s.fp) == 0;
                stream->s.fp = NULL;
            }
            break;
#if HAVE_ZLIB
        case EC_IO_GZIP:
            if (stream->s.gz) {
                res = gzclose(stream->s.gz) == Z_OK;
                stream->s.gz = NULL;
            }
            break;
#endif
        default:
            assert(!"stream type valid");
    }
    if (!stream->stdstream) {
        free(stream);
    }
    return res ? EC_SUCCESS : EC_FAILURE;
}

int
ec_io_printf(ec_io_stream *stream, const char *fmt, ...)
{
    int res = -1;
    va_list ap;
    va_start(ap, fmt);
    switch (stream->type) {
        case EC_IO_STDIO:
            res = vfprintf(stream->s.fp, fmt, ap);
            break;
#if HAVE_ZLIB
        case EC_IO_GZIP:
            res = gzvprintf(stream->s.gz, fmt, ap);
            break;
#endif
        default:
            assert(!"stream type valid");
    }
    va_end(ap);
    return res;
}

size_t
ec_io_read(void *ptr, size_t size, size_t nmemb, ec_io_stream *stream)
{
    switch (stream->type) {
        case EC_IO_STDIO:
            return fread(ptr, size, nmemb, stream->s.fp);
#if HAVE_ZLIB
        case EC_IO_GZIP:
            return gzread(stream->s.gz, ptr, size * nmemb) / size;
#endif
    }
    assert(!"stream type valid");
    return 0;
}

size_t
ec_io_write(const void *ptr, size_t size, size_t nmemb, ec_io_stream *stream)
{
    switch (stream->type) {
        case EC_IO_STDIO:
            return fwrite(ptr, size, nmemb, stream->s.fp);
#if HAVE_ZLIB
        case EC_IO_GZIP:
            return gzwrite(stream->s.gz, ptr, size * nmemb) / size;
#endif
    }
    assert(!"stream type valid");
    return 0;
}

char *
ec_io_gets(char *s, int size, ec_io_stream *stream)
{
    switch (stream->type) {
        case EC_IO_STDIO:
            return fgets(s, size, stream->s.fp);
#if HAVE_ZLIB
        case EC_IO_GZIP:
            return gzgets(stream->s.gz, s, size);
#endif
    }
    assert(!"stream type valid");
    return NULL;
}

long
ec_io_getline(char **lineptr, size_t *n, ec_io_stream *stream)
{
    char buf[4097];
    size_t len, total = 0;

#if defined(_GNU_SOURCE) || _POSIX_C_SOURCE >= 200809L || _XOPEN_SOURCE >= 700
    if (stream->type == EC_IO_STDIO) {
        return getline(lineptr, n, stream->s.fp);
    }
#endif

    do {
        if (!ec_io_gets(buf, sizeof buf, stream)) {
            if (!total) {
                return -1;
            }
            break;
        }
        len = strlen(buf);
        if (!*lineptr || *n < total + len) {
            void *tmp = realloc(*lineptr, total + len);
            if (!tmp) {
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
ec_io_seek(ec_io_stream *stream, long offset, int whence)
{
    switch (stream->type) {
        case EC_IO_STDIO:
            return fseek(stream->s.fp, offset, whence);
#if HAVE_ZLIB
        case EC_IO_GZIP:
            return gzseek(stream->s.gz, offset, whence) >= 0 ? 0 : -1;
#endif
    }
    assert(!"stream type valid");
    return -1;
}

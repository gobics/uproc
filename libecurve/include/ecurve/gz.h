#ifndef EC_GZ_H
#define EC_GZ_H

/** \file ecurve/gz.h
 *
 * Macros to replace calls to zlib's gz* functions by their stdio equivalents
 *
 */

#define EC_GZ_BUFSZ (512 * (1 << 10))

#if HAVE_ZLIB
#include <zlib.h>
#else
typedef FILE *gzFile;
#define gzread(stream, buf, n) ((int)fread(buf, 1, n, stream))
#define gzwrite(stream, buf, n) ((int)fwrite(buf, 1, n, stream))
#define gzgets(stream, s, size) fgets(s, size, stream)
#define gzopen fopen
#define gzclose fclose
#define gzprintf fprintf
#define gzputc(stream, c) putc(c, stream)
#endif

#if ZLIB_VERNUM < 0x1235
#define gzbuffer(file, size) 0
#endif

#endif

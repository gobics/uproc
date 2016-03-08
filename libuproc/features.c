/* Info about compile-time features
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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdbool.h>

#if HAVE_ZLIB_H
#include <zlib.h>
#endif

#include "uproc/features.h"

void uproc_features_print(uproc_io_stream *stream)
{
    uproc_io_printf(stream, "libuproc version %s\n", uproc_features_version());
    uproc_io_printf(stream, "zlib:   %s\n", uproc_features_zlib_version());
    uproc_io_printf(stream, "OpenMP: %d\n", uproc_features_openmp());
    uproc_io_printf(stream, "mmap:   %s\n",
                    uproc_features_mmap() ? "yes" : "no");
    uproc_io_printf(stream, "LAPACK: %s\n",
                    uproc_features_lapack() ? "yes" : "no");
}

const char *uproc_features_version(void)
{
    return PACKAGE_VERSION;
}

bool uproc_features_zlib(void)
{
    return HAVE_ZLIB_H;
}

const char *uproc_features_zlib_version(void)
{
#if HAVE_ZLIB_H
    return ZLIB_VERSION;
#else
    return "no";
#endif
}

bool uproc_features_mmap(void)
{
#if HAVE_MMAP && USE_MMAP
    return true;
#else
    return false;
#endif
}

int uproc_features_openmp(void)
{
#if _OPENMP
    return _OPENMP;
#else
    return 0;
#endif
}

bool uproc_features_lapack(void)
{
#if HAVE_LAPACKE_DGESV
    return true;
#else
    return false;
#endif
}

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

/** \file uproc/features.h
 *
 * Module: \ref grp_features
 *
 * \weakgroup grp_features
 * \{
 */

#ifndef UPROC_FEATURES_H
#define UPROC_FEATURES_H

#include <stdbool.h>

#include "uproc/io.h"

/** Print formatted info about features */
void uproc_features_print(uproc_io_stream *stream);

/** Version name */
const char *uproc_features_version(void);

/** Check zlib support */
bool uproc_features_zlib(void);

/** zlib's version string */
const char *uproc_features_zlib_version(void);

/** Check mmap() support */
bool uproc_features_mmap(void);

/** Obtain OpenMP version
 *
 * \return
 * the value of \c _OPENMP, or 0 if compiled without OpenMP.
 */
int uproc_features_openmp(void);

/** Check LAPACK availability */
bool uproc_features_lapack(void);

/**
 * \} grp_features
 */
#endif

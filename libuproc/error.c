/* Error handling facilities
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "uproc/error.h"

static int error_num;
static char error_loc[256], error_msg[256];
#if _OPENMP
#pragma omp threadprivate(error_num, error_loc, error_msg)
#endif

static uproc_error_handler *error_handler = NULL;
static void *error_context = NULL;


static const char *error_strs[] = {
        [UPROC_SUCCESS] = "success",
        [UPROC_FAILURE] = "unspecified error",
        [UPROC_ENOMEM] = "memory allocation failed",
        [UPROC_EINVAL] = "invalid argument",
        [UPROC_ENOENT] = "no such object",
        [UPROC_EIO] = "I/O error",
        [UPROC_EEXIST] = "object already exists",
};

int uproc_error_(enum uproc_error_code num, const char *func, const char *file,
                 int line, const char *fmt, ...)
{
    va_list ap;
    error_num = num;
    snprintf(error_loc, sizeof error_loc - 1, "%s() [%s:%d]", func, file, line);
    va_start(ap, fmt);
    if (fmt) {
        vsnprintf(error_msg, sizeof error_msg - 1, fmt, ap);
    } else {
        error_msg[0] = '\0';
    }
    va_end(ap);
    if (num == UPROC_SUCCESS) {
        return 0;
    }
    if (error_handler) {
        error_handler(num, error_msg, error_loc, error_context);
    }
    return -1;
}

int *uproc_error_errno_(void)
{
    return &error_num;
}

void uproc_perror(const char *fmt, ...)
{
    va_list ap;
#ifndef NDEBUG
    fprintf(stderr, "%s: ", error_loc);
#endif
    va_start(ap, fmt);
    if (fmt && *fmt) {
        vfprintf(stderr, fmt, ap);
        fputs(": ", stderr);
    }
    if (error_msg[0]) {
        fprintf(stderr, "%s: ", error_msg);
    }
    if (error_num == UPROC_ERRNO) {
        perror("");
    } else {
        fprintf(stderr, "%s\n", error_strs[error_num]);
    }
    va_end(ap);
}

const char *uproc_error_errmsg_(void)
{
    return error_msg;
}

const char *uproc_error_errloc_(void)
{
    return error_loc;
}

void
uproc_error_set_handler(uproc_error_handler *handler, void *context)
{
    error_handler = handler;
	error_context = context;
}

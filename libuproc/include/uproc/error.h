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

/** \file uproc/error.h
 *
 * Error handling facilities.
 *
 * This module provides functions and macros to retrieve information about
 * errors that may have occured.
 *
 * \weakgroup grp_error
 * \{
 */

#ifndef UPROC_ERROR_H
#define UPROC_ERROR_H

#include "uproc/common.h"


/** Available error codes */
enum uproc_error_code
{
    /** Success */
    UPROC_SUCCESS = 0,

    /** General failure */
    UPROC_FAILURE,

    /** A system call (that sets `errno`) returned an error */
    UPROC_ERRNO,

    /** Memory allocation failed */
    UPROC_ENOMEM,

    /** Invalid argument */
    UPROC_EINVAL,

    /** Object doesn't exist  */
    UPROC_ENOENT,

    /** Object already exists */
    UPROC_EEXIST,

    /** Input/output error */
    UPROC_EIO,

    /** Operation not supported */
    UPROC_ENOTSUP,
};


/* Set error information */
int uproc_error_(enum uproc_error_code num, const char *func, const char *file,
                 int line, const char *fmt, ...);


/** Set ::uproc_errno with a custom message
 *
 * Example:
 * \code
 * void *foo = malloc(sz);
 * if (!foo) {
 *     uproc_error_msg(UPROC_EINVAL, "can't allocate foo with size %zu", sz);
 *     return -1;
 *  }
 * \endcode
 * \hideinitializer
 * \param num   error code
 * \param ...   printf-style format string and corresponding arguments
 *
 */
#define uproc_error_msg(num, ...) \
    uproc_error_(num, __func__, __FILE__, __LINE__, __VA_ARGS__)


/* Get pointer to thread-local uproc_errno */
int *uproc_error_errno_(void);


/** Set ::uproc_errno with a standard message */
#define uproc_error(num) uproc_error_msg(num, NULL)


/** `errno`-like error indicator
 *
 * Like the original `errno`, evaluates to an (assignable) lvalue of type int,
 * used as an error indicator. This should usually be one of the values of
 * ::uproc_error_code.
 */
#define uproc_errno (*(uproc_error_errno_()))


/* Get thread-local uproc_errmsg */
const char *uproc_error_errmsg_(void);


/** Error message
 *
 * Evaluates to a `const char*` containing a description of the last occured
 * error.
 */
#define uproc_errmsg (uproc_error_errmsg_())


/* Get thread-local uproc_errloc */
const char *uproc_error_errloc_(void);


/** Error location
 *
 * Evaluates to a `const char*` containing the location (source file, function
 * and line number) from where ::uproc_error_ was called the last time.
 */
#define uproc_errloc (uproc_error_errloc_())


/** Print error message to stderr
 *
 * If `fmt` is a nonempty string, format it using the other arguments an
 * prepend the result, followed by a colon and space, to the error message.
 *
 * \param fmt   printf-style format string
 * \param ...   format string arguments
 */
void uproc_perror(const char *fmt, ...);


/** Error handler type
 *
 * \param num   error code
 * \param msg   error message
 * \param loc   source file and line
 */
typedef void uproc_error_handler(enum uproc_error_code num, const char *msg,
                                 const char *loc);


/** Set error handler
 *
 * Set an error handler to be called every time libuproc encounters an error.
 *
 * A simple error handler that just prints the message and exits the program
 * could look like this:
 *
 * \code
 * void handler(enum uproc_error_code num, const char *msg, const char *loc)
 * {
 *     uproc_perror("");
 *     exit(EXIT_FAILURE);
 * }
 * \endcode
 */
void uproc_error_set_handler(uproc_error_handler *);

/**
 * \}
 */
#endif

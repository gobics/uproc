#ifndef UPROC_ERROR_H
#define UPROC_ERROR_H

#include "uproc/common.h"

/** \file uproc/error.h
 *
 * Error handling
 */

enum {
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

int uproc_error_(int num, const char *func, const char *file, int line,
                const char *fmt, ...);

#define uproc_error_msg(num, ...) uproc_error_(num, __func__, __FILE__, \
                                                  __LINE__, __VA_ARGS__)
#define uproc_error(num) uproc_error_msg(num, NULL)

int *uproc_error_errno_(void);
#define uproc_errno (*(uproc_error_errno_()))

void uproc_perror(const char *fmt, ...);

const char *uproc_error_errmsg_(void);
#define uproc_errmsg (uproc_error_errmsg_())
#endif

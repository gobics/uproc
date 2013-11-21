#ifndef UPRO_ERROR_H
#define UPRO_ERROR_H

#include "upro/common.h"

/** \file upro/error.h
 *
 * Error handling
 */

enum {
    /** Success */
    UPRO_SUCCESS = 0,

    /** General failure */
    UPRO_FAILURE,

    /** A system call (that sets `errno`) returned an error */
    UPRO_ERRNO,

    /** Memory allocation failed */
    UPRO_ENOMEM,

    /** Invalid argument */
    UPRO_EINVAL,

    /** Object doesn't exist  */
    UPRO_ENOENT,

    /** Object already exists */
    UPRO_EEXIST,

    /** Input/output error */
    UPRO_EIO,

    /** Operation not supported */
    UPRO_ENOTSUP,
};

int upro_error_(int num, const char *func, const char *file, int line,
                const char *fmt, ...);

#define upro_error_msg(num, ...) upro_error_(num, __func__, __FILE__, \
                                                  __LINE__, __VA_ARGS__)
#define upro_error(num) upro_error_msg(num, NULL)

int *upro_error_errno_(void);
#define upro_errno (*(upro_error_errno_()))

void upro_perror(const char *fmt, ...);

const char *upro_error_errmsg_(void);
#define upro_errmsg (upro_error_errmsg_())
#endif

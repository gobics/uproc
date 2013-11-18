#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "upro/error.h"

int upro_error_num;
char upro_error_loc[256], upro_error_msg[256];
#pragma omp threadprivate(upro_error_num, upro_error_loc, upro_error_msg)

static const char *upro_error_errmsg[] = {
    [UPRO_SUCCESS]  = "success",
    [UPRO_FAILURE]  = "unspecified error",
    [UPRO_ENOMEM]   = "memory allocation failed",
    [UPRO_EINVAL]   = "invalid argument",
    [UPRO_ENOENT]   = "no such object",
    [UPRO_EIO]      = "I/O error",
    [UPRO_EEXIST]   = "object already exists",
    [UPRO_ESYSCALL] = "call to OS function failed"
};

int
upro_error_(int num, const char *func, const char *file, int line,
            const char *fmt, ...)
{
    va_list ap;
    upro_error_num = num;
    snprintf(upro_error_loc, sizeof upro_error_loc - 1, "%s() [%s:%d]", func,
             file, line);

    va_start(ap, fmt);
    if (fmt) {
        vsnprintf(upro_error_msgstr, sizeof upro_error_msgstr - 1, fmt, ap);
    }
    else {
        strncpy(upro_error_msgstr, upro_error_errmsg[num],
                sizeof upro_error_msgstr - 1);
    }
    va_end(ap);
    return UPRO_FAILURE;
}

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "uproc/error.h"

static int error_num;
static char error_loc[256], error_msg[256];
#if HAVE_OPENMP
#pragma omp threadprivate(error_num, error_loc, error_msg)
#endif

static const char *error_strs[] = {
    [UPROC_SUCCESS]  = "success",
    [UPROC_FAILURE]  = "unspecified error",
    [UPROC_ENOMEM]   = "memory allocation failed",
    [UPROC_EINVAL]   = "invalid argument",
    [UPROC_ENOENT]   = "no such object",
    [UPROC_EIO]      = "I/O error",
    [UPROC_EEXIST]   = "object already exists",
    [UPROC_ERRNO] = "call to OS function failed"
};

int
uproc_error_(int num, const char *func, const char *file, int line,
            const char *fmt, ...)
{
    va_list ap;
    error_num = num;
    snprintf(error_loc, sizeof error_loc - 1, "%s() [%s:%d]", func, file, line);
    va_start(ap, fmt);
    if (fmt) {
        vsnprintf(error_msg, sizeof error_msg - 1, fmt, ap);
    }
    else {
        error_msg[0] = '\0';
    }
    va_end(ap);
    return num == UPROC_SUCCESS ? 0 : -1;
}

int *
uproc_error_errno_(void)
{
    return &error_num;
}

void
uproc_perror(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (fmt && *fmt) {
        vfprintf(stderr, fmt, ap);
        fputs(": ", stderr);
    }
    fputs(error_strs[error_num], stderr);
    if (error_msg[0]) {
        fprintf(stderr, ": %s", error_msg);
    }
    if (error_num == UPROC_ERRNO) {
        fputs(": ", stderr);
        perror("");
    }
    else {
        fputc('\n', stderr);
    }
    va_end(ap);
}

const char *
uproc_error_errmsg_(void)
{
    return error_msg;
}

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "upro/error.h"

static int error_num;
static char error_loc[256], error_msg[256];
#if HAVE_OPENMP
#pragma omp threadprivate(error_num, error_loc, error_msg)
#endif

static const char *error_strs[] = {
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
    return num == UPRO_SUCCESS ? UPRO_SUCCESS : UPRO_FAILURE;
}

int *
upro_error_errno_(void)
{
    return &error_num;
}

void
upro_perror(const char *fmt, ...)
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
    if (error_num == UPRO_ESYSCALL) {
        fputs(": ", stderr);
        perror("");
    }
    else {
        fputc('\n', stderr);
    }
    va_end(ap);
}

const char *
upro_error_errmsg_(void)
{
    return error_msg;
}

AC_PREREQ(2.62)
AC_INIT([uproc], [0.0.0], [uproc@gobics.de])
AM_INIT_AUTOMAKE([foreign])
LT_PREREQ([2.2])
LT_INIT

AC_CONFIG_SRCDIR([libuproc/ecurve.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_RANLIB
AM_PROG_CC_C_O

# Needed for %zu in printf/sscanf when compiling for Windows
AC_DEFINE([__USE_MINGW_ANSI_STDIO], [1], [Use mingw stdio])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h stdint.h stdlib.h string.h])
AC_CHECK_HEADERS([unistd.h zlib.h getopt.h])

AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_VPRINTF

AC_CHECK_FUNCS([atexit munmap pow strchr strerror posix_madvise getopt_long])

# Checks for libraries
AC_SEARCH_LIBS([log2], [m])
AC_SEARCH_LIBS([gzopen], [z])

AC_OPENMP

# Check for the "check" unit testing library.
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4], [have_check=yes], [have_check=no])
AM_CONDITIONAL([HAVE_CHECK], [test x$have_check = xyes])

AC_CONFIG_FILES([Makefile
                 libuproc/Makefile
                 libuproc/include/Makefile
                 libuproc/tests/Makefile
                 libuproc/tests/data/Makefile
                 uproc-makedb/Makefile])
AC_OUTPUT

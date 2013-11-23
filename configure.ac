AC_PREREQ(2.59)
AC_INIT([uproc], [0.1], [uproc@gobics.de])
AM_INIT_AUTOMAKE([foreign])

AC_CONFIG_SRCDIR([libuproc/ecurve.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_RANLIB
AM_PROG_CC_C_O

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h stdint.h stdlib.h string.h unistd.h])
AC_CHECK_HEADERS([zlib.h])
AC_CHECK_HEADERS([omp.h])

AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_VPRINTF

AC_CHECK_FUNCS([atexit munmap pow strchr strerror])

# Checks for libraries
AC_SEARCH_LIBS([gzopen], [z])

AC_OPENMP

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

AC_CONFIG_FILES([Makefile src/Makefile libuproc/Makefile tests/Makefile])
AC_OUTPUT

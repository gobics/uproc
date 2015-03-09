AC_PREREQ(2.62)
AC_INIT([uproc], [1.2.0], [uproc@gobics.de])

# GNU make specific constructs are covered by AX_CHECK_GNU_MAKE, so don't warn about them
AM_INIT_AUTOMAKE([foreign check-news -Wall -Wno-portability])

AX_CHECK_GNU_MAKE

LT_PREREQ([2.2])
LT_INIT

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([libuproc/ecurve.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_C___ATTRIBUTE__
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_LIBTOOL_WIN32_DLL

# Enable "most reasonable" warnings
AX_CFLAGS_WARN_ALL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h inttypes.h limits.h stdint.h stdlib.h string.h])
AC_CHECK_HEADERS([unistd.h zlib.h getopt.h time.h])

AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MMAP
AC_FUNC_VPRINTF
AX_FUNC_MKDIR

AC_CHECK_FUNCS([atexit munmap pow strchr strerror posix_madvise getopt_long])

# Checks for libraries
AC_SEARCH_LIBS([log2], [m])
AC_SEARCH_LIBS([gzopen], [z])

# Checks for clock_gettime()
AC_SEARCH_LIBS([clock_gettime], [rt])
AC_CHECK_FUNCS([clock_gettime])

AC_OPENMP

# Check for the "check" unit testing library.
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4], [have_check=yes], [have_check=no])
AM_CONDITIONAL([HAVE_CHECK], [test x$have_check = xyes])

AC_ARG_ENABLE([shared],
              AS_HELP_STRING([--enable-shared],
                             [Build (and install) libuproc.so and header files]))
AM_CONDITIONAL([SHARED_LIBUPROC], [test "x$enable_shared" = "xyes"])

# Check if the mmap database format should be used.
# Using mmap has the advantage that the database doesn't have to be "parsed",
# which can save a lot of time if UProC is run several consecutive times,
# and the memory can be shared between multiple processes.
# While this works great on Linux, on OSX and possibly other operating systems,
# random access on a mmaped file residing on a HDD causes serious performance
# issues. On an SSD this seems not to be the case, so we let users decide to
# enable this feature.
AC_CANONICAL_HOST
case $host_os in
	linux*)
		enable_mmap_default=yes
		;;
	*)
		enable_mmap_default=no
		;;
esac

AC_ARG_ENABLE([mmap],
			  AS_HELP_STRING([--enable-mmap],
							 [Use the mmap database format [default=yes only on GNU/Linux]]),
			  [],
			  [enable_mmap=$enable_mmap_default])

if test "x$enable_mmap" = "xyes"; then
AC_DEFINE([USE_MMAP], [1], [Define to 1 if you want to use the mmap database format])
fi

# Check for Doxygen
test -z "$DOXYGEN" && AC_CHECK_PROGS([DOXYGEN], [doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

# define UPROC_VERSION
UPROC_GIT_VERSION

AC_CONFIG_FILES([Makefile
                 libuproc/Makefile
                 libuproc/include/Makefile
                 libuproc/tests/Makefile
                 libuproc/tests/data/Makefile
                 libuproc/docs/doxyfile
                 libuproc/docs/Makefile])
AC_OUTPUT

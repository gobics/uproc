AM_CPPFLAGS = -I$(top_srcdir)/libuproc/include
AM_CFLAGS = @CHECK_CFLAGS@
LDADD = $(top_builddir)/libuproc/libuproc.a @CHECK_LIBS@
TESTS = ck_uproc
check_PROGRAMS = ck_uproc
ck_uproc_SOURCES = $(top_builddir)/libuproc/libuproc.a \
	   ck_uproc.c ck_common.c ck_alphabet.c

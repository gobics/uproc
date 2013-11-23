AM_CPPFLAGS = -I$(top_srcdir)/libupro/include
AM_CFLAGS = @CHECK_CFLAGS@
LDADD = $(top_builddir)/libupro/libupro.a @CHECK_LIBS@
TESTS = ck_upro
check_PROGRAMS = ck_upro
ck_upro_SOURCES = $(top_builddir)/libupro/libupro.a \
	   ck_upro.c ck_common.c ck_alphabet.c

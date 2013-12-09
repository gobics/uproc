AM_CPPFLAGS = -I$(top_srcdir)/libuproc/include -I$(top_srcdir) -I$(top_srcdir)/libuproc
LDADD = $(top_builddir)/libuproc/libuproc.la

bin_PROGRAMS = uproc-makedb

uproc_makedb_SOURCES = makedb.h makedb.c build_ecurves.c calib.c progress.c
uproc_makedb_CFLAGS = $(OPENMP_CFLAGS)

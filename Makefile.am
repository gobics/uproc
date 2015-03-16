ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = libuproc

bin_PROGRAMS = uproc-dna uproc-prot uproc-import uproc-export uproc-orf uproc-makedb
noinst_LTLIBRARIES = libcommon.la

AM_CPPFLAGS = -I$(top_srcdir)/libuproc/include
LDADD = libcommon.la libuproc/libuproc.la

libcommon_la_SOURCES = common.c common.h ppopts.c ppopts.h

uproc_dna_SOURCES = main.c
uproc_dna_CPPFLAGS = $(AM_CPPFLAGS) -DMAIN_DNA=1
uproc_dna_CFLAGS = $(OPENMP_CFLAGS)

uproc_prot_SOURCES = main.c
uproc_prot_CFLAGS = $(OPENMP_CFLAGS)

uproc_import_SOURCES = import.c

uproc_export_SOURCES = export.c

uproc_orf_SOURCES = orf.c

uproc_makedb_SOURCES = makedb.c
uproc_makedb_CFLAGS = $(OPENMP_CFLAGS)

dist_doc_DATA = README.rst

analyze :
	$(MAKE) -C libuproc analyze

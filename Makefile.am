ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libuproc makedb

bin_PROGRAMS = uproc-dna uproc-prot uproc-import uproc-export uproc-orf

AM_CPPFLAGS = -I$(top_srcdir)/libuproc/include
LDADD = $(top_builddir)/libuproc/libuproc.la

uproc_dna_SOURCES = main.c uproc_opt.h
uproc_dna_CPPFLAGS = $(AM_CPPFLAGS) -DMAIN_DNA=1
uproc_dna_CFLAGS = $(OPENMP_CFLAGS)

uproc_prot_SOURCES = main.c uproc_opt.h
uproc_prot_CFLAGS = $(OPENMP_CFLAGS)

uproc_import_SOURCES = import.c uproc_opt.h
uproc_import_CPPFLAGS = $(AM_CPPFLAGS) -DPROGNAME=\"uproc-import\"

uproc_export_SOURCES = import.c uproc_opt.h
uproc_export_CPPFLAGS = $(AM_CPPFLAGS) -DEXPORT=1 -DPROGNAME=\"uproc-export\"

uproc_orf_SOURCES = orf.c uproc_opt.h

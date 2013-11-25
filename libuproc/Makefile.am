SUBDIRS = include tests

AM_CFLAGS = -Werror $(OPENMP_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/libuproc/include

lib_LTLIBRARIES = libuproc.la
libuproc_la_SOURCES = alphabet.c  bst.c  codon.c  dnaclass.c  ecurve.c  error.c  fasta.c \
				  io.c  matrix.c  mmap.c  orf.c  protclass.c  storage.c  substmat.c  word.c

BUILT_SOURCES = codon_tables.h

noinst_PROGRAMS = gen_ct
gen_ct_SOURCES = gen_codon_tables.c

codon_tables.h : gen_ct$(EXEEXT)
	./$< > $@

CLEANFILES = codon_tables.h gen_ct$(EXEEXT)

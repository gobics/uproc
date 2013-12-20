SUBDIRS = include tests
if HAVE_DOXYGEN
SUBDIRS += docs
endif

AM_CFLAGS = $(OPENMP_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/libuproc/include

lib_LTLIBRARIES = libuproc.la
libuproc_la_SOURCES = alphabet.c \
					bst.c \
					codon.c \
					dnaclass.c \
					ecurve.c \
					ecurve_internal.h \
					ecurve_mmap.c \
					ecurve_storage.c \
					error.c \
					fasta.c \
					idmap.c \
					io.c \
					matrix.c \
					orf.c \
					protclass.c \
					substmat.c \
					word.c

libuproc_la_LDFLAGS = -no-undefined

BUILT_SOURCES = codon_tables.h

noinst_PROGRAMS = gen_ct
gen_ct_SOURCES = gen_codon_tables.c

codon_tables.h : gen_ct$(EXEEXT)
	./$< > $@

CLEANFILES = codon_tables.h gen_ct$(EXEEXT)

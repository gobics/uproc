SUBDIRS = tests

if SHARED_LIBUPROC
SUBDIRS += include
if HAVE_DOXYGEN
SUBDIRS += docs
endif
lib_LTLIBRARIES = libuproc.la
else
noinst_LTLIBRARIES = libuproc.la
endif

AM_CFLAGS = $(OPENMP_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/libuproc/include

libuproc_la_SOURCES = alphabet.c \
					bst.c \
					codon.c \
					dict.c \
					dnaclass.c \
					ecurve.c \
					ecurve_internal.h \
					ecurve_mmap.c \
					ecurve_storage.c \
					error.c \
					features.c \
					idmap.c \
					io.c \
					list.c \
					matrix.c \
					orf.c \
					protclass.c \
					seqio.c \
					substmat.c \
					word.c \
					codon_tables.h \
					database.c \
					model.c

# TODO: increment version-info to 2:0:0 before release
libuproc_la_LDFLAGS = -no-undefined -version-info 1:0:1

BUILT_SOURCES = codon_tables.h
EXTRA_DIST = codon_tables.h

codon_tables.h :
	$(CC) $(AM_CPPFLAGS) $(AM_CFLAGS) -o gen_ct$(EXEEXT) gen_codon_tables.c
	./gen_ct$(EXEEXT) > $@

CLEANFILES = gen_ct$(EXEEXT)

@ifGNUmake@analyze_srcs = $(filter %.c,$(libuproc_la_SOURCES))
@ifGNUmake@analyze_plists = $(patsubst %.c,%.plist,$(analyze_srcs))
@ifGNUmake@
@ifGNUmake@MOSTLYCLEANFILES = $(analyze_plists)
@ifGNUmake@
@ifGNUmake@analyze :
@ifGNUmake@	$(MAKE) mostlyclean
@ifGNUmake@	$(MAKE) codon_tables.h
@ifGNUmake@	$(MAKE) OPENMP_CFLAGS= CC=clang do-analyze
@ifGNUmake@
@ifGNUmake@do-analyze : $(analyze_plists)
@ifGNUmake@
@ifGNUmake@$(analyze_plists): %.plist: %.c
@ifGNUmake@	@echo "  CCSA  " $@
@ifGNUmake@	$(COMPILE) --analyze $< -o $@
@ifGNUmake@
@ifGNUmake@.PHONY: analyze

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
					dnaclass.c \
					ecurve.c \
					ecurve_internal.h \
					ecurve_mmap.c \
					ecurve_storage.c \
					error.c \
					idmap.c \
					io.c \
					list.c \
					matrix.c \
					orf.c \
					protclass.c \
					seqio.c \
					substmat.c \
					word.c \
					codon_tables.h

libuproc_la_LDFLAGS = -no-undefined -version-info 0:0:0

BUILT_SOURCES = codon_tables.h
EXTRA_DIST = codon_tables.h

codon_tables.h :
	cc -o gen_ct$(EXEEXT) gen_codon_tables.c
	./gen_ct$(EXEEXT) > $@

CLEANFILES = gen_ct$(EXEEXT)


analyze_srcs = $(filter %.c,$(libuproc_la_SOURCES))
analyze_plists = $(patsubst %.c,%.plist,$(analyze_srcs))

MOSTLYCLEANFILES = $(analyze_plists)

analyze :
	$(MAKE) mostlyclean
	$(MAKE) codon_tables.h
	$(MAKE) OPENMP_CFLAGS= CC=clang do-analyze

do-analyze : $(analyze_plists)

$(analyze_plists): %.plist: %.c
	@echo "  CCSA  " $@
	$(COMPILE) --analyze $< -o $@

.PHONY: analyze

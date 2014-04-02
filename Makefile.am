ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = libuproc

bin_PROGRAMS = uproc-dna uproc-prot uproc-detailed uproc-import uproc-export uproc-orf uproc-makedb

AM_CPPFLAGS = -I$(top_srcdir)/libuproc/include
LDADD = $(top_builddir)/libuproc/libuproc.la

if IN_GIT_REPO
git_tag := $(shell git describe --abbrev=0)
git_commit := $(shell git rev-list --max-count=1 --abbrev-commit HEAD)
git_branch := $(shell git rev-parse --abbrev-ref HEAD)
AM_CPPFLAGS += -DGIT_TAG=\"$(git_tag)\" -DGIT_COMMIT=\"$(git_commit)\" -DGIT_BRANCH=\"$(git_branch)\"
endif

uproc_dna_SOURCES = main.c common.c common.h
uproc_dna_CPPFLAGS = $(AM_CPPFLAGS) -DMAIN_DNA=1
uproc_dna_CFLAGS = $(OPENMP_CFLAGS)

uproc_prot_SOURCES = main.c common.c common.h
uproc_prot_CFLAGS = $(OPENMP_CFLAGS)

uproc_detailed_SOURCES = detailed.c common.c common.h

uproc_import_SOURCES = import.c common.c common.h
uproc_import_CPPFLAGS = $(AM_CPPFLAGS)

uproc_export_SOURCES = import.c common.c common.h
uproc_export_CPPFLAGS = $(AM_CPPFLAGS) -DEXPORT=1

uproc_orf_SOURCES = orf.c common.c common.h

uproc_makedb_SOURCES = makedb/makedb.h makedb/makedb.c makedb/build_ecurves.c \
					makedb/calib.c  makedb/progress.c \
					common.c common.h

uproc_makedb_CPPFLAGS = -I$(top_srcdir)/libuproc/include -I$(top_srcdir)/libuproc
uproc_makedb_CFLAGS = $(OPENMP_CFLAGS)

doc_DATA = INSTALL README NEWS AUTHORS COPYING COPYING.LESSER

analyze :
	$(MAKE) -C libuproc analyze

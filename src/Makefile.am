bin_PROGRAMS = uproc-dna \
			   uproc-prot \
			   uproc-orf

AM_CPPFLAGS = -I$(top_srcdir)/libuproc/include
LDADD = $(top_builddir)/libuproc/libuproc.a

uproc_dna_SOURCES = main.c
uproc_dna_CPPFLAGS = $(AM_CPPFLAGS) -DMAIN_DNA=1
uproc_dna_CFLAGS = $(OPENMP_CFLAGS)

uproc_prot_SOURCES = main.c
uproc_prot_CFLAGS = $(OPENMP_CFLAGS)

uproc_orf_SOURCES = orf.c

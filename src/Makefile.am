bin_PROGRAMS = upro-dna \
			   upro-prot \
			   upro-orf

AM_CPPFLAGS = -I$(top_srcdir)/libupro/include
LDADD = ../libupro/libupro.a

upro_dna_SOURCES = main.c
upro_dna_CPPFLAGS = $(AM_CPPFLAGS) -DMAIN_DNA=1
upro_dna_CFLAGS = $(OPENMP_CFLAGS)

upro_prot_SOURCES = main.c
upro_prot_CFLAGS = $(OPENMP_CFLAGS)

upro_orf_SOURCES = orf.c

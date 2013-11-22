AM_CPPFLAGS = -I$(top_srcdir)/libupro/include
AM_CFLAGS = -Werror $(OPENMP_CFLAGS)

noinst_LIBRARIES = libupro.a
libupro_a_SOURCES = alphabet.c  bst.c  codon.c  dnaclass.c  ecurve.c  error.c  fasta.c \
				  io.c  matrix.c  mmap.c  orf.c  protclass.c  storage.c  substmat.c  word.c


orf.o : codon_tables.h

codon_tables.h : gen_codon_tables.c codon.c
	$(CC) -Iinclude $(CFLAGS) $(CPFLAGS) -o gen_ct $^
	./gen_ct > $@
	rm gen_ct

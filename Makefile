include config.mk

MODULES := ecurve alphabet substmat word storage bst classify orf codon matrix seqio

ifeq ($(HAVE_MMAP), yes)
MODULES += mmap
CPPFLAGS += -DHAVE_MMAP
endif

EXECUTABLES := main-dna main-prot
ifeq ($(HAVE_OPENMP), yes)
EXECUTABLES += main-dna-omp main-prot-omp
endif


OBJECTS := $(addprefix $(OBJDIR)/,$(addsuffix .o, $(MODULES)))
HEADERS := $(INCDIR)/ecurve.h $(INCDIR)/ecurve/common.h $(addprefix $(INCDIR)/ecurve/,$(addsuffix .h, $(MODULES)))

CPPFLAGS += -I$(INCDIR)

TESTSOURCES := $(wildcard t/*.test.c)
TESTFILES := $(TESTSOURCES:.c=.t)

default : archive $(EXECUTABLES)

archive : $(ARCHIVE)

main-dna : $(SRCDIR)/main.c $(OBJECTS)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -DMAIN_DNA -o $@ $^ $(LIBS)

main-prot : $(SRCDIR)/main.c $(OBJECTS)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -DMAIN_PROT -o $@ $^ $(LIBS)

main-prot-omp : $(SRCDIR)/main_omp.c $(OBJECTS)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -fopenmp -DMAIN_PROT -o $@ $^ $(LIBS)

main-dna-omp : $(SRCDIR)/main_omp.c $(OBJECTS)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -fopenmp -DMAIN_DNA -o $@ $^ $(LIBS)

test : $(TESTFILES)
	@prove -Q -e "" || echo "some tests failed. run 'make test-verbose' for detailed output"

test-verbose : $(TESTFILES)
	@prove -fo --directives -e ""

$(OBJDIR) :
	@-mkdir $@

$(OBJDIR)/%.o : $(SRCDIR)/%.c $(HEADERS) | $(OBJDIR)
	@echo CC -c $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/orf.o : $(SRCDIR)/codon_tables.h

$(SRCDIR)/codon_tables.h : $(SRCDIR)/gen_codon_tables.c $(SRCDIR)/codon.c
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o gen_codon_tables $^
	@./gen_codon_tables > $@
	@rm gen_codon_tables

t/%.t : t/%.c t/test.c $(ARCHIVE)
	@echo CC $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^

$(ARCHIVE) : $(OBJECTS)
	@echo AR $@
	@$(AR) $(ARFLAGS) $@ $?
	@echo RANLIB $@
	@$(RANLIB) $@

clean : clean-obj clean-test clean-doc
	@rm -f $(SRCDIR)/codon_tables.h

clean-test :
	@rm -f $(TESTFILES)

clean-obj :
	@rm -rf $(OBJDIR)

clean-doc :
	@rm -rf doc

doc :
	@doxygen Doxyfile

convert :
	cd convert && $(MAKE)

.PHONY : clean clean-obj test test-verbose clean-test doc clean-doc convert

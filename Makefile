include config.mk

MODULES := ecurve alphabet substmat word storage bst classify orf codon matrix seqio

ifeq ($(HAVE_MMAP), yes)
MODULES += mmap
CPPFLAGS += -DHAVE_MMAP
endif

ifeq ($(HAVE_OPENMP), yes)
MAIN_SOURCE := main_omp.c
CFLAGS += -fopenmp
else
MAIN_SOURCE := main.c
endif


OBJECTS := $(addprefix $(OBJDIR)/,$(addsuffix .o, $(MODULES)))
HEADERS := $(INCDIR)/ecurve.h $(INCDIR)/ecurve/common.h $(addprefix $(INCDIR)/ecurve/,$(addsuffix .h, $(MODULES)))

CPPFLAGS += -I$(INCDIR)

TESTSOURCES := $(wildcard t/*.test.c)
TESTFILES := $(TESTSOURCES:.c=.t)

default : archive main-dna main-prot

archive : $(ARCHIVE)

main-dna : $(SRCDIR)/$(MAIN_SOURCE) $(OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DMAIN_DNA -o $@ $^ $(LIBS)

main-prot : $(SRCDIR)/$(MAIN_SOURCE) $(OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DMAIN_PROT -o $@ $^ $(LIBS)

test : $(TESTFILES)
	@prove -Q -e "" || echo "some tests failed. run 'make test-verbose' for detailed output"

test-verbose : $(TESTFILES)
	@prove -fo --directives -e ""

$(OBJDIR) :
	$%mkdir $@

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
	@rm $(SRCDIR)/codon_tables.h

clean-test :
	@rm -f $(TESTFILES)

clean-obj :
	@rm -rf $(OBJDIR)

clean-doc :
	@rm -rf doc

doc :
	@doxygen Doxyfile

.PHONY : clean clean-obj test test-verbose clean-test doc clean-doc

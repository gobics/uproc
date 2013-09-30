include ../config.mk

MODULES := ecurve alphabet substmat word storage bst classify orf codon matrix seqio mmap

DESTDIR ?= .

OBJECTS := $(addprefix $(OBJDIR)/,$(addsuffix .o, $(MODULES)))
HEADERS := $(INCDIR)/ecurve.h $(INCDIR)/ecurve/common.h $(addprefix $(INCDIR)/ecurve/,$(addsuffix .h, $(MODULES)))

CPPFLAGS += -I$(INCDIR)
CFLAGS += -fPIC

TESTSOURCES := $(wildcard t/*.test.c)
TESTFILES := $(TESTSOURCES:.c=.t)

default : $(ARCHIVENAME)

test : $(TESTFILES)
	@prove -Q -e "" || echo "some tests failed. run 'make test-verbose' for detailed output"

test-verbose : $(TESTFILES)
	@prove -fo --directives -e ""

$(OBJDIR) :
	@-mkdir $@

$(OBJDIR)/%.o : $(SRCDIR)/%.c $(HEADERS) | $(OBJDIR)
	@echo CC -c $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $< $(LDFLAGS) $(LIBS)

$(OBJDIR)/orf.o : $(SRCDIR)/codon_tables.h

$(SRCDIR)/codon_tables.h : $(SRCDIR)/gen_codon_tables.c $(SRCDIR)/codon.c
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o gen_codon_tables $^ $(LDFLAGS) $(LIBS)
	@./gen_codon_tables > $@
	@rm gen_codon_tables

t/%.t : t/%.c t/test.c $(ARCHIVENAME)
	@echo CC $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(ARCHIVENAME) : $(OBJECTS)
	@echo AR $@
	@$(AR) $(ARFLAGS) $@ $?
	@echo RANLIB $@
	@$(RANLIB) $@

clean : clean-obj clean-test clean-doc
	@rm -f $(ARCHIVENAME)
	@rm -f $(SRCDIR)/codon_tables.h

clean-test :
	@rm -f $(TESTFILES)

clean-obj :
	@rm -rf $(OBJDIR)

clean-doc :
	@rm -rf doc

doc :
	@doxygen Doxyfile

.PHONY : clean clean-obj test test-verbose clean-test doc clean-doc

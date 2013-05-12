include config.mk

MODULES := ecurve alphabet distmat word storage
OBJECTS := $(addprefix $(OBJDIR)/,$(addsuffix .o, $(MODULES)))
HEADERS := $(INCDIR)/ecurve.h $(addprefix $(INCDIR)/ecurve/,$(addsuffix .h, $(MODULES)))

CPPFLAGS += -I$(INCDIR)

TESTSOURCES := $(wildcard t/*.test.c)
TESTFILES := $(TESTSOURCES:.c=.t)

default : archive

archive : $(ARCHIVE)

test : $(TESTFILES)
	@prove -Q -e "" || echo "some tests failed. run 'make test-verbose' for detailed output"

test-verbose : $(TESTFILES)
	@prove -fo -e ""

$(OBJDIR) :
	$%mkdir $@

$(OBJDIR)/%.o : $(SRCDIR)/%.c $(HEADERS) | $(OBJDIR)
	@echo CC -c $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

t/%.t : t/%.c t/test.c $(ARCHIVE)
	@echo CC $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^

$(ARCHIVE) : $(OBJECTS)
	@echo AR $@
	@$(AR) $(ARFLAGS) $@ $?
	@echo RANLIB $@
	@$(RANLIB) $@


clean : clean-obj clean-test clean-doc

clean-test :
	@rm -f $(TESTFILES)

clean-obj :
	@rm -rf $(OBJDIR)

clean-doc :
	@rm -rf doc

doc :
	@doxygen Doxyfile

.PHONY : clean clean-obj test test-verbose clean-test doc clean-doc

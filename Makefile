include config.mk

CPPFLAGS += -Ilibecurve/$(INCDIR)

ARCHIVE := libecurve/$(ARCHIVENAME)
TARGETS = ecurve-dna ecurve-prot ecurve-convert ecurve-build ecurve-calib

MATLAB_ROOT := /opt/matlab
MATLAB_ARCH := glnxa64
MATLAB_INCDIR := $(MATLAB_ROOT)/extern/include
MATLAB_LIBDIR := $(MATLAB_ROOT)/bin/$(MATLAB_ARCH)

ifeq ($(HAVE_MATLAB), yes)
TARGETS += ecurve-mat2bin
endif


default : $(TARGETS)


ecurve-dna : main.c $(ARCHIVE)
	@echo CC $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -DMAIN_DNA -o $@ $^ $(LIBS)

ecurve-prot : main.c $(ARCHIVE)
	@echo CC $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -DMAIN_PROT -o $@ $^ $(LIBS)


ecurve-mat2bin : mat2bin.c $(ARCHIVE)
	@echo CC $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -I$(MATLAB_INCDIR) -o $@ $^ -L$(MATLAB_LIBDIR) $(LIBS) -lmat

ecurve-% : %.c $(ARCHIVE)
	@echo CC $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LIBS)

$(ARCHIVE) :
	@$(MAKE) -C libecurve $(ARCHIVENAME)

clean :
	rm -f $(TARGETS)
	$(MAKE) -C libecurve clean

.PHONY : clean

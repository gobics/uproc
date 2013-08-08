include ../config.mk

MATLAB_ROOT := /opt/matlab
MATLAB_ARCH := glnxa64
MATLAB_INCDIR := $(MATLAB_ROOT)/extern/include
MATLAB_LIBDIR := $(MATLAB_ROOT)/bin/$(MATLAB_ARCH)

CPPFLAGS += -I../$(INCDIR)
LIBS += ../$(ARCHIVE)

EXECUTABLES := ecurve-convert
ifeq ($(HAVE_MATLAB), yes)
EXECUTABLES += ecurve-mat2bin
endif

default : $(EXECUTABLES)

ecurve-convert : convert.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LIBS)

ecurve-mat2bin : mat2bin.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -I$(MATLAB_INCDIR) -o $@ $^ -L$(MATLAB_LIBDIR) $(LIBS) -lmat


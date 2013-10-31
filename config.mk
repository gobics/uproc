HAVE_MATLAB ?= no
HAVE_MMAP ?= yes
HAVE_OPENMP ?= yes
HAVE_ZLIB ?= yes

ARCHIVENAME := libupro.a

CC ?= cc
CPPFLAGS ?= -D_GNU_SOURCE
CFLAGS ?= -std=c99 -pedantic -Wall -Wextra -Os
#CFLAGS += -O0 -g
#CFLAGS += -pg
#CFLAGS += -fprofile-arcs -ftest-coverage

LIBS ?= -lm

AR ?= ar
ARFLAGS ?= rv
RANLIB ?= ranlib

OBJDIR := obj
SRCDIR := src
INCDIR := include

ifeq ($(HAVE_MMAP), yes)
CPPFLAGS += -DHAVE_MMAP=1
endif
ifeq ($(HAVE_OPENMP), yes)
CFLAGS += -fopenmp
endif
ifeq ($(HAVE_ZLIB), yes)
CPPFLAGS += -DHAVE_ZLIB=1 -DHAVE_FILENO=1
LIBS += -lz
endif

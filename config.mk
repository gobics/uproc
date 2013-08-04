HAVE_MMAP ?= yes
HAVE_OPENMP ?= yes

ARCHIVE := ecurve.a

CC ?= cc
CPPFLAGS ?=
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

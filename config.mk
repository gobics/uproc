HAVE_MMAP ?= yes

ARCHIVE := ecurve.a

CC ?= cc
CPPFLAGS ?=
CFLAGS ?= -std=c99 -pedantic -Wall -Wextra -O2
#CFLAGS += -O0 -g
#CFLAGS += -pg
#CFLAGS += -fprofile-arcs -ftest-coverage

AR ?= ar
ARFLAGS ?= rv
RANLIB ?= ranlib

OBJDIR := obj
SRCDIR := src
INCDIR := include

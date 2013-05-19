HAVE_MMAP ?= yes

ARCHIVE := ecurve.a

CC ?= cc
CPPFLAGS ?=
CFLAGS ?= -std=c99 -pedantic -Wall -Wextra -g

AR ?= ar
ARFLAGS ?= rv
RANLIB ?= ranlib

OBJDIR := obj
SRCDIR := src
INCDIR := include

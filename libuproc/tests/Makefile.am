if HAVE_CHECK
TESTS = ck_uproc
check_PROGRAMS = ck_uproc
ck_uproc_SOURCES = ck_uproc.c \
				ck_common.c \
				ck_alphabet.c \
				ck_idmap.c

ck_uproC_CFLAGS = @CHECK_CFLAGS@
ck_uproc_CPPFLAGS = -I$(top_srcdir)/libuproc/include \
				-DDATADIR=\"$(abs_top_srcdir)/libuproc/tests/data/\"
ck_uproc_LDADD = $(top_builddir)/libuproc/libuproc.la @CHECK_LIBS@
endif

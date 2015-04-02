if HAVE_CHECK
SUBDIRS = data
TESTS = ck_common \
		ck_alphabet \
		ck_bst \
		ck_codon \
		ck_dict \
		ck_idmap \
		ck_list \
		ck_matrix \
		ck_mosaic \
		ck_util \
		ck_word


check_PROGRAMS = $(TESTS)

AM_CFLAGS = @CHECK_CFLAGS@ $(COVERAGE_FLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/libuproc/include \
				-DDATADIR=\"$(abs_top_srcdir)/libuproc/tests/data/\" \
				-DTMPDATADIR=\"$(abs_top_builddir)/libuproc/tests/data/\"
LDADD = $(top_builddir)/libuproc/libuproc.la @CHECK_LIBS@
AM_LDFLAGS =  $(COVERAGE_LDFLAGS)
endif

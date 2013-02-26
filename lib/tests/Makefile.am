noinst_PROGRAMS = tests

AM_CFLAGS = -I$(top_srcdir)/include

LIBCBZ = $(top_builddir)/src/libcbz.la

tests_SOURCES = main.c

tests_LDADD = $(LIBCBZ)
tests_DEPENDENCES = $(LIBCBZ)
tests_LDFLAGS = -no-install

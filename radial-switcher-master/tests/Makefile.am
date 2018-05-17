## Process this file with automake to produce Makefile.in

TESTS = check_switcher_util
check_PROGRAMS = check_switcher_util
check_switcher_util_SOURCES = check_switcher-util.c $(top_builddir)/src/switcher-util.h
check_switcher_util_CFLAGS = @CHECK_CFLAGS@ @COMPIZ_CFLAGS@
check_switcher_util_LDADD = @CHECK_LIBS@ @COMPIZ_LIBS@ $(top_builddir)/src/libswitcher-util.la

## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libgalaxy_la_LDFLAGS = $(PFLAGS)
libgalaxy_la_LIBADD = @COMPIZ_LIBS@
nodist_libgalaxy_la_SOURCES = galaxy_options.c  \
			        galaxy_options.h
dist_libgalaxy_la_SOURCES = \
	galaxy.c \
	particle.c

BUILT_SOURCES = $(nodist_libgalaxy_la_SOURCES)

INCLUDES =                              \
	@COMPIZ_CFLAGS@                  \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          \
	-I$(top_srcdir)/include

moduledir = $(plugindir)


module_LTLIBRARIES = libgalaxy.la

CLEANFILES = *_options.c *_options.h

galaxy_options.h: ../../metadata/galaxy.xml.in
		$(BCOP_BIN) --header $@ $<

galaxy_options.c: ../../metadata/galaxy.xml.in galaxy_options.h
		$(BCOP_BIN) --source $@ $<

AM_CFLAGS = -I$(top_srcdir)/src $(CHECK_CFLAGS) $(REDLAND_CFLAGS) $(RASQAL_CFLAGS) $(RAPTOR_CFLAGS) $(WARNING_CFLAGS)
AM_LDFLAGS = $(CHECK_LIBS)

check_PROGRAMS = check_utils
TESTS = $(check_PROGRAMS)

.tc.c:
	checkmk $< > $@ || rm -f $@

check_utils_SOURCES = check_utils.tc $(top_builddir)/src/globals.c $(top_builddir)/src/utils.c $(top_srcdir)/src/redstore.h
check_utils_LDADD = $(top_builddir)/src/redhttp/libredhttp.la

# FIXME: could this list be made automatically?
CLEANFILES = check_utils.c
CLEANFILES += *.gcov *.gcda *.gcno

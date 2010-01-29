AM_CFLAGS = @CHECK_CFLAGS@
AM_LDFLAGS = @CHECK_LIBS@
TESTS = 
check_PROGRAMS = 

if HAVE_CHECK
check_PROGRAMS += check_httpd
TESTS += $(check_PROGRAMS)
endif

%.c: %.t
	checkmk $< > $@

EXTRA_DIST = check_httpd.t
BUILT_SOURCES = check_httpd.c
check_httpd_SOURCES = check_httpd.c $(top_builddir)/src/http/redhttpd.h
check_httpd_LDADD = $(top_builddir)/src/http/libredhttpd.a

AM_CFLAGS = @REDLAND_CFLAGS@ @CHECK_CFLAGS@
AM_LDFLAGS = @REDLAND_LIBS@ @CHECK_LIBS@
TESTS = 
check_PROGRAMS = 

if HAVE_CHECK
check_PROGRAMS += check_request check_uri
TESTS += $(check_PROGRAMS)
endif

%.c: %.tc
	checkmk $< > $@

check_request_SOURCES = check_request.tc $(top_builddir)/src/http/redhttpd.h
nodist_check_request_SOURCES = check_request.c
check_request_LDADD = $(top_builddir)/src/http/libredhttpd.a

check_uri_SOURCES = check_uri.tc $(top_builddir)/src/http/redhttpd.h
nodist_check_uri_SOURCES = check_uri.c
check_uri_LDADD = $(top_builddir)/src/http/libredhttpd.a

CLEANFILES = check_uri.c check_request.c

AM_CFLAGS = @CHECK_CFLAGS@
AM_LDFLAGS = @CHECK_LIBS@
TESTS = 
check_PROGRAMS = 

if HAVE_CHECK
check_PROGRAMS += check_headers check_request check_url
TESTS += $(check_PROGRAMS)
endif

%.c: %.tc
	checkmk $< > $@

check_headers_SOURCES = check_headers.tc
nodist_check_headers_SOURCES = check_headers.c
check_headers_LDADD = $(top_builddir)/src/redhttp/libredhttp.a

check_request_SOURCES = check_request.tc
nodist_check_request_SOURCES = check_request.c
check_request_LDADD = $(top_builddir)/src/redhttp/libredhttp.a

check_url_SOURCES = check_url.tc
nodist_check_url_SOURCES = check_url.c
check_url_LDADD = $(top_builddir)/src/redhttp/libredhttp.a

CLEANFILES = check_headers.c check_request.c check_url.c

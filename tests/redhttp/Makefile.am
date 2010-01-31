AM_CFLAGS = @CHECK_CFLAGS@
AM_LDFLAGS = @CHECK_LIBS@
TESTS = 
check_PROGRAMS = 

if HAVE_CHECK
check_PROGRAMS += check_headers check_request check_response check_server check_url
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

check_response_SOURCES = check_response.tc
nodist_check_response_SOURCES = check_response.c
check_response_LDADD = $(top_builddir)/src/redhttp/libredhttp.a

check_server_SOURCES = check_server.tc
nodist_check_server_SOURCES = check_server.c
check_server_LDADD = $(top_builddir)/src/redhttp/libredhttp.a

check_url_SOURCES = check_url.tc
nodist_check_url_SOURCES = check_url.c
check_url_LDADD = $(top_builddir)/src/redhttp/libredhttp.a

CLEANFILES = check_headers.c check_request.c check_response.c check_server.c check_url.c

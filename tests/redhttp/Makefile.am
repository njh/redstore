AM_CFLAGS = @CHECK_CFLAGS@ -DFIXTURE_DIR=\"$(top_srcdir)/tests/redhttp/\" -I$(top_srcdir)/src
AM_LDFLAGS = @CHECK_LIBS@

check_PROGRAMS = check_headers check_negotiate check_request check_response check_server check_url
TESTS = $(check_PROGRAMS)

.tc.c:
	checkmk $< > $@ || rm -f $@

check_headers_SOURCES = check_headers.tc $(top_srcdir)/src/redhttp/redhttp.h
check_headers_LDADD = $(top_builddir)/src/redhttp/libredhttp.la

check_negotiate_SOURCES = check_negotiate.tc $(top_srcdir)/src/redhttp/redhttp.h
check_negotiate_LDADD = $(top_builddir)/src/redhttp/libredhttp.la

check_request_SOURCES = check_request.tc $(top_srcdir)/src/redhttp/redhttp.h
check_request_LDADD = $(top_builddir)/src/redhttp/libredhttp.la

check_response_SOURCES = check_response.tc $(top_srcdir)/src/redhttp/redhttp.h
check_response_LDADD = $(top_builddir)/src/redhttp/libredhttp.la

check_server_SOURCES = check_server.tc $(top_srcdir)/src/redhttp/redhttp.h
check_server_LDADD = $(top_builddir)/src/redhttp/libredhttp.la

check_url_SOURCES = check_url.tc $(top_srcdir)/src/redhttp/redhttp.h
check_url_LDADD = $(top_builddir)/src/redhttp/libredhttp.la

EXTRA_DIST = \
    http_request_09.txt \
    http_request_crlf.txt \
    http_request_lf.txt \
    http_request_no_url.txt \
    http_request_post_invalid.txt \
    http_request_post_truncated.txt \
    http_request_post.txt \
    http_request_with_spaces.txt

# FIXME: could this list be made automatically?
CLEANFILES = check_headers.c check_negotiate.c check_request.c check_response.c check_server.c check_url.c
CLEANFILES += *.gcov *.gcda *.gcno

AM_CFLAGS = $(WARNING_CFLAGS)

noinst_LTLIBRARIES = libredhttp.la
libredhttp_la_SOURCES = \
  headers.c \
  negotiate.c \
  redhttp.h \
  redhttp_private.h \
  request.c \
  response.c \
  server.c \
  url.c

noinst_PROGRAMS = test_redhttpd
test_redhttpd_SOURCES = test_redhttpd.c
test_redhttpd_LDADD = libredhttp.la

CLEANFILES = *.gcov *.gcda *.gcno

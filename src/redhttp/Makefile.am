
noinst_LIBRARIES = libredhttp.a
libredhttp_a_SOURCES = \
  headers.c \
  redhttp.h \
  request.c \
  response.c \
  server.c \
  url.c

noinst_PROGRAMS = test_redhttpd
test_redhttpd_SOURCES = test_redhttpd.c
test_redhttpd_LDADD = libredhttp.a

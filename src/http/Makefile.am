
noinst_LIBRARIES = libredhttpd.a
libredhttpd_a_SOURCES = \
  headers.c \
  redhttpd.h \
  request.c \
  response.c \
  server.c \
  uri.c

noinst_PROGRAMS = test_redhttpd
test_redhttpd_SOURCES = test_redhttpd.c
test_redhttpd_LDADD = libredhttpd.a

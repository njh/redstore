AM_CFLAGS = @LIBMICROHTTPD_CFLAGS@ @REDLAND_CFLAGS@
AM_LDFLAGS = @LIBMICROHTTPD_LIBS@ @REDLAND_LIBS@

bin_PROGRAMS = redstore
redstore_SOURCES = \
  redstore.c \
  redstore.h \
  graphs.c \
  formatters.c \
  pages.c \
  query.c \
  images.c \
  util.c

noinst_PROGRAMS = test_nano_httpd
test_nano_httpd_SOURCES = test_nano_httpd.c nano_httpd.c nano_httpd.h

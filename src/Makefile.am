AM_CFLAGS = @REDLAND_CFLAGS@
AM_LDFLAGS = @REDLAND_LIBS@

bin_PROGRAMS = redstore
redstore_LDADD = http/libredhttpd.a
redstore_SOURCES = \
  redstore.c \
  redstore.h \
  graphs.c \
  formatters.c \
  pages.c \
  query.c \
  images.c \
  util.c

SUBDIRS = http

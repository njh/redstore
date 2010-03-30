AM_CFLAGS = @REDLAND_CFLAGS@
AM_LDFLAGS = @REDLAND_LIBS@

bin_PROGRAMS = redstore
redstore_LDADD = redhttp/libredhttp.la
redstore_SOURCES = \
  redstore.c \
  redstore.h \
  graphs.c \
  formatters.c \
  load.c \
  dump.c \
  pages.c \
  query.c \
  images.c

SUBDIRS = redhttp

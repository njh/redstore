AM_CFLAGS = @REDLAND_CFLAGS@
AM_LDFLAGS = @REDLAND_LIBS@

bin_PROGRAMS = redstore
redstore_LDADD = redhttp/libredhttp.la
redstore_SOURCES = \
  redstore.c \
  redstore.h \
  data.c \
  graphs.c \
  description.c \
  formatters.c \
  load.c \
  pages.c \
  query.c \
  images.c

SUBDIRS = redhttp

CLEANFILES = *.gcov *.gcda *.gcno

AM_CFLAGS = $(REDLAND_CFLAGS) $(WARNING_CFLAGS)
AM_LDFLAGS = $(REDLAND_LIBS)

bin_PROGRAMS = redstore
redstore_LDADD = redhttp/libredhttp.la
redstore_SOURCES = \
  data.c \
  description.c \
  formatters.c \
  genid.c \
  graphs.c \
  images.c \
  pages.c \
  query.c \
  redstore.c \
  redstore.h \
  update.c

SUBDIRS = redhttp

CLEANFILES = *.gcov *.gcda *.gcno

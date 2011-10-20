AM_CFLAGS = $(REDLAND_CFLAGS) $(RASQAL_CFLAGS) $(RAPTOR_CFLAGS) $(WARNING_CFLAGS)

bin_PROGRAMS = redstore
redstore_LDADD = redhttp/libredhttp.la $(REDLAND_LIBS) $(RASQAL_LIBS) $(RAPTOR_LIBS)
redstore_SOURCES = \
  data.c \
  description.c \
  formatters.c \
  genid.c \
  graphs.c \
  globals.c \
  images.c \
  pages.c \
  query.c \
  redstore.c \
  redstore.h \
  update.c \
  utils.c

SUBDIRS = redhttp

CLEANFILES = *.gcov *.gcda *.gcno

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I build-scripts
SUBDIRS = src .

if HAVE_CHECK
SUBDIRS += tests
endif

test: check

INDENT = indent -npro -kr -fca -blf -ip1 -l100 -lc100 -nut -ts2 -i2 -c0 -nfca -nsob -hnl -v
indent:
	find $(top_srcdir)/src -type f -name "*.[ch]" -print -exec $(INDENT) {} ';'

dist_noinst_SCRIPTS = todo.pl

dist-hook:
	perl $(top_srcdir)/todo.pl > $(distdir)/TODO
	if test -e $(top_srcdir)/.git; then \
	  git log --date-order $(top_srcdir) > $(distdir)/ChangeLog; \
	fi

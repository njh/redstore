AUTOMAKE_OPTIONS = foreign
SUBDIRS = src .

if HAVE_CHECK
SUBDIRS += tests
endif

test: check

INDENT = indent -npro -kr -nsob -fca -blf -ip1 -hnl -l100 -lc100 -nut -ts4 -v
indent:
	find $(top_srcdir)/src -type f -name "*.[ch]" -print -exec $(INDENT) {} ';'

dist_noinst_SCRIPTS = todo.pl

dist-hook:
	perl $(top_srcdir)/todo.pl > $(distdir)/TODO
	if test -e $(top_srcdir)/.svn; then \
		svn log --stop-on-copy $(top_srcdir) > $(distdir)/ChangeLog; \
	fi

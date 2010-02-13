AUTOMAKE_OPTIONS = foreign
SUBDIRS = src .

if HAVE_CHECK
SUBDIRS += tests
endif

test: check

INDENT = indent -npro -kr -nsob -fca -blf -ip1 -hnl -l100 -lc100 -nut -ts4 -v
indent:
	find $(top_builddir)/src -type f -name "*.[ch]" -print -exec $(INDENT) {} ';'

dist-hook:
	perl todo.pl > $(distdir)/TODO
	svn log --stop-on-copy > $(distdir)/ChangeLog

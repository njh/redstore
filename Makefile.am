ACLOCAL_AMFLAGS = -I build-scripts
SUBDIRS = src .

dist_man_MANS = redstore.1
EXTRA_DIST = redstore.1.md

if HAVE_PANDOC
redstore.1: redstore.1.md
	pandoc -s -f markdown -t man -o $@ $<
endif

if HAVE_CHECK
SUBDIRS += tests
endif

test: check

INDENT = indent -npro -kr -fca -blf -ip1 -l100 -lc100 -nut -ts2 -i2 -c0 -nfca -nsob -hnl -v
indent:
	find $(top_srcdir)/src -type f -name "*.[ch]" -print -exec $(INDENT) {} ';'

dist_noinst_SCRIPTS = todo.pl

dist-hook:
	$(PERL) $(top_srcdir)/todo.pl > $(distdir)/TODO
	if test -e $(top_srcdir)/.git; then \
	  git log --date-order $(top_srcdir) > $(distdir)/ChangeLog; \
	fi

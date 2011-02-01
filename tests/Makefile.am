AM_CFLAGS = $(WARNING_CFLAGS)
SUBDIRS = redhttp .

dist_check_SCRIPTS = integration_test.pl
dist_check_DATA = test001.rdf foaf.ttl
TESTS = $(dist_check_SCRIPTS)
TESTS_ENVIRONMENT = \
	REDSTORE="$(top_builddir)/src/redstore" \
	perl -w -Mstrict -MTest::Harness -e "runtests(@ARGV)"

CLEANFILES = *.gcov *.gcda *.gcno

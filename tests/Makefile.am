AM_CFLAGS = $(WARNING_CFLAGS)
SUBDIRS = redhttp .

dist_check_SCRIPTS = \
  redstore-http_test.pl \
  sparql11-http-rdf-update_test.pl \
  sparql11-protocol_test.pl
dist_check_DATA = test001.rdf foaf.ttl redstore_testlib.pm
TESTS = $(dist_check_SCRIPTS)
TESTS_ENVIRONMENT = \
	REDSTORE="$(top_builddir)/src/redstore" \
	perl -w -Mstrict -MTest::Harness -I "$(top_builddir)/tests" -e "runtests(@ARGV)"

CLEANFILES = *.gcov *.gcda *.gcno

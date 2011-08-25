dist_check_SCRIPTS = \
  redstore-http_test.pl \
  sparql11-http-rdf-update_test.pl \
  sparql11-protocol_test.pl \
  redstore-storage_test.pl

dist_check_DATA = \
  test001.rdf \
  foaf.ttl \
  foaf.json \
  foaf.nt \
  redstore_testlib.pm

TESTS = $(dist_check_SCRIPTS)
TESTS_ENVIRONMENT = \
	REDSTORE="$(top_builddir)/src/redstore" \
	perl -w -Mstrict -MTest::Harness -I "$(srcdir)" -e "runtests(@ARGV)"

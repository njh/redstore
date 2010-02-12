SUBDIRS = redhttp .

dist_check_SCRIPTS = integration_test.pl
dist_check_DATA = test001.rdf
TESTS = $(dist_check_SCRIPTS)
TESTS_ENVIRONMENT = perl -w -Mstrict -MTest::Harness -e "runtests(@ARGV)"

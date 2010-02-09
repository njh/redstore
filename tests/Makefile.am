SUBDIRS = redhttp .

check_SCRIPTS = integration_test.pl
TESTS = $(check_SCRIPTS)
TESTS_ENVIRONMENT = perl -w -Mstrict -MTest::Harness -e "runtests(@ARGV)"
CLEANFILES = $(check_SCRIPTS)

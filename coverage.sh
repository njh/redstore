#!/bin/sh
#
# Create an HTML Coverage Report using lcov
#

if ! which lcov &>/dev/null; then
    echo "lcov is not installed."
    exit -1
fi

if ! which genhtml &>/dev/null; then
    echo "genhtml is not installed."
    exit -1
fi


cd `dirname $0`

if [ ! -d coverage ]; then
    mkdir coverage
fi

if [ -e Makefile ]; then
    make clean
fi

find . -name '*.gcda' -delete
find . -name '*.gcno' -delete
find . -name '*.gcov' -delete
lcov --directory src --zerocounters

if ! ./configure --enable-debug CFLAGS='-fprofile-arcs -ftest-coverage'; then
    echo "Failed to configure project; aborting."
    exit -1
fi

if ! make check; then
    echo "Some tests failed; aborting."
    exit -1
fi

lcov --directory src --capture --output-file coverage.info
genhtml -t "RedStore Coverage Report" --output-directory coverage coverage.info

#!/usr/bin/perl

use strict;
use warnings;

use Test::More tests => 1;

# FIXME: we should try again if the first random port number doesn't work
my $HTTP_PORT = 10000 + ((rand(2**16) + time()) % 10000);
print "port: $HTTP_PORT\n";


is(1,1);

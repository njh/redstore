#!/usr/bin/env perl
#
# Script to pretty format issues from Github into a TODO list.
#
# Nicholas J Humfrey <njh@aelius.com>
# 

use JSON qw(decode_json);
use strict;

my $json = `curl -s https://api.github.com/repos/njh/redstore/issues?state=open`;
die "Couldn't get todo list from GitHub" unless defined $json;
my $data = decode_json($json);

print "Open issues on GitHub\n";
print "---------------------\n";
foreach my $issue (@$data) {
    printf("[%2.2d] %s\n", $issue->{'number'}, $issue->{'title'});
}

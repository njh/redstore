#!/usr/bin/env perl
#
# Script to pretty format issues from Github into a TODO list.
#
# Nicholas J Humfrey <njh@aelius.com>
# 

use LWP::Simple;
use JSON qw(decode_json);
use Data::Dumper;
use strict;

my $json = decode_json(
    get("http://github.com/api/v2/json/issues/list/njh/redstore/open")
);

my @sorted = sort {
    $a->{'position'} <=> $b->{'position'}
} @{$json->{'issues'}};

# Display the titles
foreach my $issue (@sorted) {
    print '['.$issue->{'number'}.'] '.$issue->{'title'}."\n";
}

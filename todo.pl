#!/usr/bin/env perl

use LWP::Simple;
use strict;

my $csv = get("http://code.google.com/p/redstore/issues/csv");

my $issues = {};
foreach my $line (split(/[\r\n]+/, $csv)) {
    # FIXME: use the first line as an index instead
    my ($id, $type, $status, $priority, $milestone, $owner, $summary, $labels) = 
        ($line =~ /^"(\d+)","(\w*)","(\w*)","(\w*)","(.*)","(\w*)","(.*)","(.*)"/);
    if ($id && $priority && $summary) {
        $issues->{$priority}->{$id} = $summary;
    }
}

my @order = ('Critical','High','Medium','Low');
foreach my $priority (@order) {
    if ($issues->{$priority}) {
        my $title = "$priority Priority";
        print "$title\n", ('=' x length($title)), "\n";
        foreach my $id (sort {$a <=> $b} keys %{$issues->{$priority}}) {
            printf("[%3.3d] %s\n", $id, $issues->{$priority}->{$id});
        }
        print "\n";
    }
}

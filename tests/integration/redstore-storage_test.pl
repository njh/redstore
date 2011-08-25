#!/usr/bin/perl
#
# Test each of the redland storage modules with redstore
#

use redstore_testlib;
use warnings;
use strict;

use Test::More tests => 33;

# Create a libwww-perl user agent
my ($request, $response);
my $ua = new_redstore_client();


sub test_storage
{
    my ($storage_type, $storage_opts, $storage_name, $is_persistent) = @_;
    my ($pid, $base_url) = start_redstore($storage_type, $storage_opts, $storage_name, 1);

    # Check that the server process is running
    is_running($pid);

    # Test that the server really is running
    $response = $ua->get($base_url);
    is($response->code, 200, "Getting homepage is successful");

    # Test loading some data
    load_fixture('foaf.nt', $base_url."data/foaf.nt");
    
    # If the store is persistent, start a new process
    if ($is_persistent) {
      # Shut down redstore
      stop_redstore($pid);

      # Startup a new process, using the same db
      ($pid, $base_url) = start_redstore($storage_type, $storage_opts, $storage_name, 0);
    }
    
    # Test fetching it back using SELECT
    $response = $ua->get($base_url."query?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D%0D%0A&format=csv");
    is($response->code, 200, "SPARQL SELECT query is successful");
    is($response->content_type, "text/csv", "SPARQL SELECT query Content Type data is text/csv");
    my @lines = split(/[\r\n]+/,$response->content);
    is(scalar(@lines), 15, "SPARQL response contains fifteen lines");
    is($lines[0], "s,p,o", "First line of SPARQL response contains CSV header");

    # Shut down redstore
    stop_redstore($pid);
}

# In-memory list
{
    test_storage("memory");
}

# In-memory hash
{
    test_storage("hashes", "hash-type='memory'");
}

# SQLite
{
    test_storage("sqlite", undef, 'redstore-test.sqlite', 1);
    
    # Clean-up
    ok(unlink('redstore-test.sqlite'), "Deleted sqlite storage file");
}

# BDB
{
    test_storage("hashes", "hash-type='bdb',dir='.'", 'redstore-test', 1);
    
    my @dbfiles = (
      'redstore-test-contexts.db',
      'redstore-test-po2s.db',
      'redstore-test-so2p.db',
      'redstore-test-sp2o.db'
    );

    # Clean-up
    foreach my $file (@dbfiles) {
      ok(unlink($file), "Deleted storage file $file");
    }
}

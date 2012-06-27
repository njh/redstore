#!/usr/bin/perl
#
# Test each of the redland storage modules with redstore
#

use redstore_testlib;
use warnings;
use strict;

use Test::More tests => 49;

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
    is($response->code, 200, "$storage_type - getting homepage is successful");

    # Test loading some data
    load_fixture('foaf.nt', $base_url."data/foaf.nt");

    # If the store is persistent, start a new process
    if ($is_persistent) {
      # Shut down redstore
      stop_redstore($pid);

      # Startup a new process, using the same db
      ($pid, $base_url) = start_redstore($storage_type, $storage_opts, $storage_name, 0);
    }

    # Test a ASK for anything in the store
    $response = $ua->get($base_url."query?query=ASK+%7B%3Fs+%3Fp+%3Fo%7D&format=json");
    is($response->code, 200, "$storage_type - SPARQL ASK query is successful");
    like($response->content, qr["boolean" : true], "$storage_type - SPARQL ASK Query result is 'true'");

    # Test fetching it back using SELECT
    $response = $ua->get($base_url."query?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D%0D%0A&format=csv");
    is($response->code, 200, "$storage_type - SPARQL SELECT query is successful");
    is($response->content_type, "text/csv", "$storage_type - SPARQL SELECT query Content Type data is text/csv");
    my @lines = split(/[\r\n]+/,$response->content);
    is(scalar(@lines), 15, "$storage_type - SPARQL response contains fifteen lines");
    is($lines[0], "s,p,o", "$storage_type - First line of SPARQL response contains CSV header");

    # Test a ASK for a specific triple in the store
    $response = $ua->get($base_url."query?query=ASK+%7B%3Chttp%3A%2F%2Fwww.example.com%2Fjoe%2Ffoaf.rdf%3E+%3Chttp%3A%2F%2Fxmlns.com%2Ffoaf%2F0.1%2Fmaker%3E+%3Chttp%3A%2F%2Fwww.example.com%2Fjoe%23me%3E%7D%0D%0A&format=json");
    is($response->code, 200, "$storage_type - SPARQL ASK query for specific triple is successful");
    like($response->content, qr["boolean" : true], "$storage_type - SPARQL ASK Query for specific triple result is 'true'");

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
    ok(unlink('redstore-test.sqlite'), "Deleting sqlite storage file");
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
      ok(unlink($file), "Deleting storage file $file");
    }
}

#!/usr/bin/perl
#
# RedStore tests for:
#
# SPARQL 1.1 Service Description
# http://www.w3.org/TR/sparql11-service-description/
#

use redstore_testlib;
use warnings;
use strict;


use Test::More tests => 7;

# Create a libwww-perl user agent
my ($request, $response, @lines);
my $ua = new_redstore_client();

# Start RedStore
my ($pid, $base_url) = start_redstore();

# Double check that the server is running
is_running($pid);


# GET the service description from the SPARQL endpoint as RDF/XML
$response = $ua->get($base_url."sparql?format=rdfxml-abbrev");
is($response->code, 200, "GET to sparql endpoint for rdfxml-abbrev is successful");
is($response->content_type, "application/rdf+xml", "GET to sparql endpoint for RDF/XML has correct MIME type");
like($response->content, qr/<sd:Service>/, "GET to sparql endpoint returns RDF/XML containing '<sd:Service>'");

# GET the service description from the SPARQL endpoint as Turtle
$response = $ua->get($base_url."sparql?format=turtle");
is($response->code, 200, "GET to sparql endpoint for turtle is successful");
like($response->content_type, qr[/turtle], "GET to sparql endpoint for Turtle has /turtle in the MIME type");
like($response->content, qr/ sd:Service /, "GET to sparql endpoint returns Turle containing 'sd:Service'");


END {
    stop_redstore($pid);
}


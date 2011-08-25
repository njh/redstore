#!/usr/bin/perl
#
# RedStore tests for:
#
# SPARQL 1.1 Uniform HTTP Protocol for Managing RDF Graphs
# http://www.w3.org/TR/sparql11-http-rdf-update/
#

use redstore_testlib;
use warnings;
use strict;


use Test::More tests => 64;

my $TEST_CASE_URI = 'http://www.w3.org/2000/10/rdf-tests/rdfcore/xmlbase/test001.rdf';
my $ESCAPED_TEST_CASE_URI = 'http%3A%2F%2Fwww.w3.org%2F2000%2F10%2Frdf-tests%2Frdfcore%2Fxmlbase%2Ftest001.rdf';

# Create a libwww-perl user agent
my ($request, $response, @lines);
my $ua = new_redstore_client();

# Start RedStore
my ($pid, $base_url) = start_redstore();

# Double check that the server is running
is_running($pid);


# Test PUTing a graph
$request = HTTP::Request->new( 'PUT', $base_url.'data/?graph='.$ESCAPED_TEST_CASE_URI );
$request->content( read_fixture('test001.rdf') );
$request->content_length( length($request->content) );
$request->content_type( 'application/rdf+xml' );
$response = $ua->request($request);
is($response->code, 200, "PUTting a data into a graph is successful");
is($response->content_type, 'text/plain', "Non negotiated PUT response is of type text/plain");
like($response->content, qr/Successfully added triples/, "Load response message contain triple count");

# Test getting a graph as default format
$request = HTTP::Request->new( 'GET', $base_url.'data/?graph='.$ESCAPED_TEST_CASE_URI );
$response = $ua->request($request);
is($response->code, 200, "Getting a graph with no format specified is successful");
is($response->content_type, "application/rdf+xml", "Default Content Type data is correct");

# Test getting a graph as Turtle
$request = HTTP::Request->new( 'GET', $base_url.'data/?graph='.$ESCAPED_TEST_CASE_URI );
$request->header('Accept', 'application/x-turtle');
$response = $ua->request($request);
is($response->code, 200, "Getting a graph as Turtle using content negotiation is successful");
is($response->content_type, "application/x-turtle", "Getting a graph as Turtle using content negotiation has correct content type");
like($response->content, qr[<http://example.org/dir/file#frag>\s+<http://example.org/value>\s+\"v\"\s*.\n], "Graph data is correct");

# Test getting a graph as N-Triples
$request = HTTP::Request->new( 'GET', $base_url.'data/?graph='.$ESCAPED_TEST_CASE_URI );
$request->header('Accept', 'text/plain');
$response = $ua->request($request);
is($response->code, 200, "Getting a graph is successful");
@lines = split(/[\r\n]+/, $response->content);
is(scalar(@lines), 1, "Number of triples is correct");

# Test getting a non-existant graph
$response = $ua->get($base_url.'data/invalid.rdf');
is($response->code, 404, "Getting a non-existant graph returns 404");
is($response->content_type, 'text/plain', "Graph not found page is of type text/plain");
is($response->content, "Graph not found.\n", "Graph not found message content is correct");
is($response->content_length, length("Graph not found.\n"), "Graph not found message content length header is correct");

# Test getting an unsupported content type
$response = $ua->get($base_url.'data/?graph='.$ESCAPED_TEST_CASE_URI."&format=unsupported");
is($response->code, 406, "Getting a graph in an unsupported format returns 406");

# Test a GET request without any arguments
$response = $ua->get($base_url.'data');
is($response->code, 400, "GETing with no arguments should be an error");
is($response->content, "Missing graph path or argument.\n", "GETing with no arguments have the right error message.");

# Test a GET request with both default and graph arguments
$response = $ua->get($base_url.'data?graph=foobar&default');
is($response->code, 400, "GET response for both default and graph arguments should be 400");

# Test POSTing some Turtle
$request = HTTP::Request->new( 'POST', $base_url.'data/foaf.rdf' );
$request->content( read_fixture('foaf.ttl') );
$request->content_length( length($request->content) );
$request->content_type( 'application/x-turtle' );
$response = $ua->request($request);
is($response->code, 200, "POSTting Tutle formatted data into a graph");
is($response->content_type, 'text/plain', "Non negotiated POST response is of type text/plain");
like($response->content, qr/Successfully added triples/, "Load response message contain triple count");

# Test POSTing some Turtle with an HTML response
$request = HTTP::Request->new( 'POST', $base_url.'data/foaf.rdf' );
$request->content( read_fixture('foaf.ttl') );
$request->content_length( length($request->content) );
$request->content_type( 'application/x-turtle' );
$request->header( 'Accept', 'text/html' );
$response = $ua->request($request);
is($response->code, 200, "POSTting Tutle formatted data into a graph");
is($response->content_type, 'text/html', "POST with HTML response is of type text/html");
like($response->content, qr/Successfully added triples/, "Load response message contain triple count");
is_valid_xhtml($response->content, "HTML Response to POSTing a graph should be valid XHTML");

# Test a head request
$response = $ua->head($base_url.'data/foaf.rdf');
is($response->code, 200, "HEAD response for graph we just created is 200");

# Test head request on indirect URL resolution
$response = $ua->head($base_url.'data/ignorethis?graph=foaf.rdf');
is($response->code, 200, "HEAD response for /data/ignorethis?graph=foaf.rdf is 200");

# Test a head request on the default graph
$response = $ua->head($base_url.'data?default');
is($response->code, 200, "HEAD response for the default graph is 200");

# Test a head request without any arguments
$response = $ua->head($base_url.'data');
is($response->code, 400, "HEAD response for request with no arguements should be 400");

# Test a head request with both default and graph arguments
$response = $ua->head($base_url.'data?graph=foobar&default');
is($response->code, 400, "HEAD response for both default and graph arguments should be 400");

# Test head request on indirect URL resolution
$response = $ua->head($base_url.'data/foo/bar/rat/rat?graph=/data/foaf.rdf');
is($response->code, 200, "HEAD response for /data/foo/bar/rat/rat?graph=/data/foaf.rdf is 200");

# Test DELETEing a graph
$request = HTTP::Request->new( 'DELETE', $base_url.'data/foaf.rdf' );
$response = $ua->request($request);
is($response->code, 200, "DELETEing a graph is successful");

# Test a head request on deleted graph
$response = $ua->head($base_url.'data/foaf.rdf');
is($response->code, 404, "HEAD response for deleted graph is 404");

# Test dumping the triplestore as N-Quads
$response = $ua->get($base_url.'data?default&format=nquads');
is($response->code, 200, "Triplestore dump successful");
is($response->content_type, 'text/x-nquads', "Triplestore dump is correct MIME type");
@lines = split(/[\r\n]+/, $response->content);
like($lines[-1], qr[^(\S+) (\S+) (\S+) (\S+) \.$], "Last line of dump looks like a quad");

# Test PUTing without any arguments
$request = HTTP::Request->new( 'PUT', $base_url.'data' );
$response = $ua->request($request);
is($response->code, 400, "PUTing with no arguments should be an error");
is($response->content, "Missing graph path or argument.\n", "PUTing with no arguments have the right error message.");

# Test a PUT request with both default and graph arguments
$request = HTTP::Request->new( 'PUT', $base_url.'data?graph=foobar&default');
$response = $ua->request($request);
is($response->code, 400, "PUT response for both default and graph arguments should be 400");

# Test replacing data with a PUT request
{
    $request = HTTP::Request->new( 'PUT', $base_url.'data/foaf.rdf' );
    $request->content( read_fixture('foaf.ttl') );
    $request->content_length( length($request->content) );
    $request->content_type( 'application/x-turtle' );
    $response = $ua->request($request);
    is($response->code, 200, "Putting data into a graph is successful");

    # Count the number of triples
    $response = $ua->get($base_url.'data/foaf.rdf', 'Accept' => 'text/plain');
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 14, "Number of triples is correct");

    # Test PUTing data into a graph
    $request = HTTP::Request->new( 'PUT', $base_url.'data/foaf.rdf' );
    $request->content( read_fixture('test001.rdf') );
    $request->content_length( length($request->content) );
    $request->content_type( 'application/rdf+xml' );
    $response = $ua->request($request);
    is($response->code, 200, "Replacing data in a graph is successful");

    # Count the number of triples
    $response = $ua->get($base_url.'data/foaf.rdf', 'Accept' => 'text/plain');
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 1, "New number of triples is correct");
};

# Test PUTing JSON
{
    $request = HTTP::Request->new( 'PUT', $base_url.'data/foaf.rdf' );
    $request->content( read_fixture('foaf.json') );
    $request->content_length( length($request->content) );
    $request->content_type( 'application/json' );
    $response = $ua->request($request);
    is($response->code, 200, "Putting JSON data into a graph is successful");

    # Count the number of triples
    $response = $ua->get($base_url.'data/foaf.rdf', 'Accept' => 'text/plain');
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 14, "Number of triples after PUTing JSON is correct");
};

# Test POSTing data into a new graph, with a graph store allocated identifier
{
    $request = HTTP::Request->new( 'POST', $base_url.'data' );
    $request->content( read_fixture('foaf.ttl') );
    $request->content_length( length($request->content) );
    $request->content_type( 'application/x-turtle' );
    $request->header( 'Accept', 'text/plain' );
    $response = $ua->request($request);
    is($response->code, 201, "POSTing data into a new graph should return status code 201");
    is($response->content_type, "text/plain", "Content Type of response is correct");
    like($response->content, qr/Successfully added triples to/, "Response messages is correct");

    my $new_uri = $response->header('Location');
    like($new_uri,  qr[^${base_url}data], "The location header should match the service endpoint URI");
    like($new_uri,  qr[/([0-9a-f]{4})-([0-9a-f]{4})$], "The new graph identifier should be in the format xxxx-xxxx");

    # Check that the new graph exists
    $response = $ua->get($new_uri, 'Accept' => 'text/plain');
    is($response->code, 200, "The new graph can be GET again");
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 14, "Number of triples in new graph is correct");
};



# Test DELETEing without any arguments
$request = HTTP::Request->new( 'DELETE', $base_url.'data' );
$response = $ua->request($request);
is($response->code, 400, "DELETEing with no arguments should be an error");
is($response->content, "Missing graph path or argument.\n", "DELETEing with no arguments have the right error message.");

# Test a DELETE request with both default and graph arguments
$request = HTTP::Request->new( 'DELETE', $base_url.'data?graph=foobar&default');
$response = $ua->request($request);
is($response->code, 400, "DELETE response for both default and graph arguments should be 400");

# Test DELETEing everything in the triplestore
{
    $request = HTTP::Request->new( 'DELETE', $base_url.'data?default' );
    $response = $ua->request($request);
    is($response->code, 200, "DELETEing all the triples is successful");

    # Count the number of triples
    $response = $ua->get($base_url.'data?default', 'Accept' => 'text/plain');
    is($response->code, 200, "Getting all the triples is successful");
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 0, "There should be no triples remaining");
};

# Test a POST request with both default and graph arguments
$request = HTTP::Request->new( 'POST', $base_url.'data?graph=foobar&default');
$response = $ua->request($request);
is($response->code, 400, "POST response for both default and graph arguments should be 400");

# Test POSTing data into the default graph
{
    $request = HTTP::Request->new( 'POST', $base_url.'data?default' );
    $request->content( read_fixture('foaf.nt') );
    $request->content_length( length($request->content) );
    $request->content_type( 'text/plain' );
    $response = $ua->request($request);
    is($response->code, 200, "POSTing data into the default graph is succcessful");
    like($response->content, qr/Successfully added triples to/, "Response messages is correct");

    # Count the number of triples
    $response = $ua->get($base_url.'data?default', 'Accept' => 'text/plain');
    is($response->code, 200, "Getting all the triples is successful");
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 14, "Number of triples in new graph is correct");
};



END {
    stop_redstore($pid);
}


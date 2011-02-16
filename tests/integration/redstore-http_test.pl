#!/usr/bin/perl
#
# Various RedStore specific tests of its HTTP interface.
#

use redstore_testlib;
use warnings;
use strict;

use Test::More tests => 80;

my $TEST_CASE_URI = 'http://www.w3.org/2000/10/rdf-tests/rdfcore/xmlbase/test001.rdf';
my $ESCAPED_TEST_CASE_URI = 'http%3A%2F%2Fwww.w3.org%2F2000%2F10%2Frdf-tests%2Frdfcore%2Fxmlbase%2Ftest001.rdf';

# Create a libwww-perl user agent
my ($request, $response, @lines);
my $ua = new_redstore_client();

# Start RedStore
my ($pid, $port) = start_redstore();
my $base_url = "http://localhost:$port/";

# Double check that the server is running
is_running($pid);

# Test getting the homepage
$response = $ua->get($base_url);
is($response->code, 200, "Getting homepage is successful");
is($response->content_type, 'text/html', "Homepage is of type text/html");
ok($response->content_length > 100, "Homepage should be more than 100 bytes long");
is_valid_xhtml($response->content, "Homepage should be valid XHTML");

# Test getting the query page
$response = $ua->get($base_url.'query');
is($response->code, 200, "Getting query page is successful");
is($response->content_type, 'text/html', "Query page is of type text/html");
ok($response->content_length > 100, "Query page is more than 100 bytes long");
is_valid_xhtml($response->content, "Query page should be valid XHTML");

# Test removing trailing slash
$response = $ua->get($base_url."query/");
is($response->code, 301, "Getting a URL with a trailing slash returns 301");
is($response->header('Location'), "/query", "Getting a URL with a trailing slash returns location without trailing slash");
is_valid_xhtml($response->content, "Redirect page should be valid XHTML");

# Test getting Service Description page
$response = $ua->get($base_url.'description', 'Accept' => 'text/html');
is($response->code, 200, "Getting Service Description page is successful");
is($response->content_type, 'text/html', "Service Description page is of type text/html");
ok($response->content_length > 100, "Service Description page is more than 100 bytes long");
is_valid_xhtml($response->content, "Service Description page should be valid XHTML");
like($response->content, qr(<h1>Service Description</h1>), "Service Description page has an h1 title");
like($response->content, qr(SPARQL 1.1), "Service Description references SPARQL 1.1");
like($response->content, qr(application/sparql), "Service Description references application/sparql");
like($response->content, qr(rdf-sparql-query), "Service Description references rdf-sparql-query");
like($response->content, qr(rdfxml), "Service Description references rdfxml");
like($response->content, qr(application/rdf\+xml), "Service Description references application/rdf+xml");
like($response->content, qr(www.w3.org/TR/rdfa/), "Service Description references www.w3.org/TR/rdfa/");
like($response->content, qr(SPARQL XML Query Results), "Service Description references SPARQL XML Query Results");
like($response->content, qr(application/sparql-results\+xml), "Service Description references application/sparql-results+xml");

# Test getting Service Description as RDF
$response = $ua->get($base_url.'description?format=rdfxml-abbrev');
is($response->code, 200, "Gettting RDF Service Description is successful");
is($response->content_type, 'application/rdf+xml', "RDF Service Description is of type application/rdf+xml");
is_wellformed_xml($response->content, "RDF Service Description is valid XML");

# Test getting the load page
$response = $ua->get($base_url.'load');
is($response->code, 200, "Getting load page is successful");
is($response->content_type, 'text/html', "Load page is of type text/html");
ok($response->content_length > 100, "Load page is more than 100 bytes long");
is_valid_xhtml($response->content, "Load page should be valid XHTML");

# Test getting the insert page
$response = $ua->get($base_url.'insert');
is($response->code, 200, "Getting insert page is successful");
is($response->content_type, 'text/html', "Insert page is of type text/html");
ok($response->content_length > 100, "Insert page is more than 100 bytes long");
is_valid_xhtml($response->content, "Insert page should be valid XHTML");

# Test getting the delete page
$response = $ua->get($base_url.'delete');
is($response->code, 200, "Getting delete page is successful");
is($response->content_type, 'text/html', "Delete page is of type text/html");
ok($response->content_length > 100, "Delete page is more than 100 bytes long");
is_valid_xhtml($response->content, "Delete page should be valid XHTML");

# Test getting the favicon
$response = $ua->get($base_url.'favicon.ico');
is($response->code, 200, "Getting favicon.ico is successful");
is($response->content_type, 'image/x-icon', "Getting favicon.ico set the content type");
ok($response->content_length > 100, "favicon.ico is more than 100 bytes long");

# Test getting robots.txt
$response = $ua->get($base_url.'robots.txt');
is($response->code, 200, "Getting robots.txt is successful");
is($response->content_type, 'text/plain', "Getting robots.txt set the content type");
ok($response->content_length > 20, "robots.txt is more than 20 bytes long");

# Test getting empty list of graphs
$response = $ua->get($base_url.'graphs');
is($response->code, 200, "Getting empty list of graphs is successful");
is($response->content_type, 'text/plain', "List of graphs page is of type text/plain");
is($response->content,'', "Empty list of grpahs returns empty page");

# Test getting empty list of graphs as HTML
$response = $ua->get($base_url.'graphs', 'Accept' => 'text/html');
is($response->code, 200, "Getting empty list of graphs as HTML is successful");
is($response->content_type, 'text/html', "List of graphs page is of type text/html");
like($response->content, qr[No named graphs.], "Page contains No Named Graphs message");
is_valid_xhtml($response->content, "No graphs message should be valid XHTML");



my $TEST_URI = $base_url."data/test001.rdf";
my $ESCAPED_TEST_URI = "http%3A%2F%2Flocalhost%3A$port%2Fdata%2Ftest001.rdf";
load_fixture('test001.rdf', $TEST_URI);

# Test getting list of graphs
$response = $ua->get($base_url.'graphs');
is($response->code, 200, "Getting list of graphs is successful");
is($response->content_type, 'text/plain', "Graph list is of type text/plain");
is($response->content, "$TEST_URI\n", "List of graphs page contains graph that was added");

# Test getting list of graphs as HTML
$response = $ua->get($base_url.'graphs', 'Accept' => 'text/html');
is($response->code, 200, "Getting list of graphs is successful");
is($response->content_type, 'text/html', "Graph list is of type text/html");
like($response->content, qr[<li><a href="/data/\?graph=$ESCAPED_TEST_URI">$TEST_URI</a></li>], "List of graphs page contains graph that was added");
is_valid_xhtml($response->content, "Graph list should be valid XHTML");

# Test POSTing a url to be loaded
{
    $ua->request(HTTP::Request->new( 'DELETE', $base_url.'data/foaf.rdf' ));
    
    $response = $ua->post( $base_url.'load', {'uri' => fixture_url('foaf.ttl'), 'graph' => $base_url.'data/foaf.rdf'});
    is($response->code, 200, "POSTing URL to load is successful");
    is($response->content_type, 'text/plain', "Non negotiated load response is of type text/plain");
    like($response->content, qr/Successfully added triples/, "Load response message contains triple count");
    
    # Count the number of triples
    $response = $ua->get($base_url.'data/foaf.rdf', 'Accept' => 'text/plain');
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 14, "Number of triples in loaded graph is correct");
}

# Test POSTing to /load without a uri
$response = $ua->post( $base_url.'load');
is($response->code, 400, "POSTing to /load without a URI should fail");
like($response->content, qr/Missing URI to load/, "Response mentions missing URI");

# Test POSTing to /load with an empty uri argument
$response = $ua->post( $base_url.'load', {'uri' => ''});
is($response->code, 400, "POSTing to /load an empty 'uri' argument should fail");
like($response->content, qr/Missing URI to load/, "Response mentions missing URI");



# Test POSTing triples to /insert
{
    $response = $ua->post( $base_url.'insert', {
        'content' => "<test:s1> <test:p2> <test:o3> .\n<test:s2> <test:p2> <test:o2> .\n",
        'content-type' => 'ntriples',
        'graph' => 'test:g'
    });
    is($response->code, 200, "POSTing data to /insert is successful");
    like($response->content, qr/Successfully added triples to test:g/, "Response messages is correct");

    # Count the number of triples
    $response = $ua->get($base_url.'data/?graph=test%3Ag', 'Accept' => 'text/plain');
    is($response->code, 200, "Getting the number of triples is successful");
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 2, "New number of triples is correct");
}

# Test POSTing to /insert without a content argument
$response = $ua->post( $base_url.'insert', {'graph' => $base_url.'data/foaf.rdf'});
is($response->code, 400, "POSTing to /insert without any content should fail");
like($response->content, qr/Missing the 'content' argument/, "Response mentions missing content argument");

# Test POSTing triples to /delete
{
    $response = $ua->post( $base_url.'delete', {
        'content' => "<test:s1> <test:p2> <test:o3> .\n",
        'content-type' => 'ntriples',
        'graph' => 'test:g'
    });
    is($response->code, 200, "POSTing data to /delete is successful");
    like($response->content, qr/Successfully deleted triples./, "Response messages is correct");

    # Count the number of triples
    $response = $ua->get($base_url.'data/?graph=test%3Ag', 'Accept' => 'text/plain');
    is($response->code, 200, "Getting the number of triples is successful");
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 1, "Number of remaining triples is correct");
}

# Test POSTing to /delete without a content argument
$response = $ua->post( $base_url.'delete');
is($response->code, 400, "POSTing to /delete without any content should fail");
like($response->content, qr/Missing the 'content' argument/, "Response mentions missing content argument");




END {
    stop_redstore($pid);
}


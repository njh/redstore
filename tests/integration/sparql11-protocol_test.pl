#!/usr/bin/perl
#
# RedStore tests for:
#
# SPARQL 1.1 Protocol for RDF
# http://www.w3.org/TR/sparql11-protocol/
#

use redstore_testlib;
use warnings;
use strict;


use Test::More tests => 67;

# Create a libwww-perl user agent
my ($request, $response, @lines);
my $ua = new_redstore_client();

# Start RedStore
my ($pid, $port) = start_redstore();
my $base_url = "http://localhost:$port/";

# Double check that the server is running
is_running($pid);



# Test a SELECT query an empty store
$response = $ua->get($base_url."query?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D%0D%0A&format=xml");
is($response->code, 200, "SPARQL SELECT query on empty store is successful");
is(scalar(@_ = split(/<result>/,$response->content))-1, 0, "SPARQL SELECT Query Result count on empty store should be 0");
is_wellformed_xml($response->content, "SPARQL response is valid XML");

# Test a CONSTRUCT query an empty store
$response = $ua->get($base_url."query?query=CONSTRUCT+%7B%3Fs+%3Fp+%3Fo%7D+WHERE+%7B%3Fs+%3Fp+%3Fo%7D&format=ntriples");
is($response->code, 200, "SPARQL CONSTRUCT query on empty store does not give an error");
is($response->content_type, 'text/plain', "SPARQL CONSTRUCT query gives the right response type");
is($response->content, '', "SPARQL CONSTRUCT query on empty store returns empty N-Triples data");

# Test an ASK query an empty store
$response = $ua->get($base_url."query?query=ASK+%7B%3Fs+%3Fp+%3Fo%7D&format=json");
is($response->code, 200, "SPARQL ASK query is successful");
is($response->content_type, "application/sparql-results+json", "SPARQL ASK query Content Type data is correct");
like($response->content, qr["boolean" : false], "SPARQL ASK Query contains right content");
is_wellformed_json($response->content, "SPARQL ASK query response is valid JSON");

# Take making an invalid query
$response = $ua->get($base_url."query?query=asdfgh");
is($response->code, 500, "Invalid SPARQL query should return error code");
is($response->content_type, 'text/plain', "The content type for an invalid SPARQL query should be text/plain");
like($response->content, qr[syntax error], "Invalid SPRQL query response should contain 'syntax error'");
like($response->content, qr[line 1], "Invalid SPRQL query response should contain the line number");

# Take making a query with an invalid result format
$response = $ua->get($base_url."query?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D%0D%0A&format=invalid");
is($response->code, 406, "Unsupported result format should return status code 406");
like($response->content, qr[Results format not supported], "Unsupported results formats should return an error message");


my $TEST_URI = $base_url."data/test001.rdf";
load_fixture('test001.rdf', $TEST_URI);

# Test getting list of graphs using SPARQL
$response = $ua->get($base_url."query?query=SELECT+DISTINCT+%3Fg+WHERE+%7BGRAPH+%3Fg+%7B+%3Fs+%3Fp+%3Fo+%7D%7D&format=csv");
is($response->code, 200, "Getting list of graphs using SPARQL is successful");
is($response->content_type, 'text/csv', "Graph list is of type text/csv");
@lines = split(/[\r\n]+/,$response->content);
is(scalar(@lines), 2, "SPARQL response contains two lines");
is($lines[0], "Result,g", "First line of SPARQL response contains CSV header");
is($lines[1], "1,uri($TEST_URI)", "Second line of SPARQL response contains graph URI");


# Test a SELECT query with an HTML response
$response = $ua->get($base_url."query?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D%0D%0A&format=html");
is($response->code, 200, "SPARQL SELECT query is successful");
is($response->content_type, "application/xhtml+xml", "SPARQL SELECT query Content Type data is correct");
like($response->content, qr[<td><span class="literal"><span class="value">v</span></span></td>], "SPARQL SELECT Query returned correct value");
is_valid_xhtml($response->content, "HTML SPARQL response should be valid XHTML");

# Test a SELECT query with an XML response
$response = $ua->get($base_url."query?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D%0D%0A&format=xml");
is($response->code, 200, "SPARQL SELECT query is successful");
is($response->content_type, "application/sparql-results+xml", "SPARQL SELECT query Content Type data is correct");
like($response->content, qr[<binding name="o"><literal>v</literal></binding>], "SPARQL SELECT Query contains right content");
is_wellformed_xml($response->content, "SPARQL response is valid XML");

# Test a ASK query with an XML response
$response = $ua->get($base_url."query?query=ASK+%7B%3Fs+%3Fp+%3Fo%7D&format=xml");
is($response->code, 200, "SPARQL ASK query is successful");
is($response->content_type, "application/sparql-results+xml", "SPARQL ASK query Content Type data is correct");
like($response->content, qr[<boolean>true</boolean>], "SPARQL ASK Query contains right content");
is_wellformed_xml($response->content, "SPARQL ASK response is valid XML");

# Test a SELECT query with a JSON response
$response = $ua->get($base_url."query?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D%0D%0A&format=json");
is($response->code, 200, "SPARQL SELECT query is successful");
is($response->content_type, "application/sparql-results+json", "SPARQL SELECT query Content Type data is correct");
like($response->content, qr[{ "type": "literal", "value": "v" }], "SPARQL SELECT Query contains right content");
is_wellformed_json($response->content, "SPARQL SELECT query response is valid JSON");

# Test a ASK query with a JSON response
$response = $ua->get($base_url."query?query=ASK+%7B%3Fs+%3Fp+%3Fo%7D&format=json");
is($response->code, 200, "SPARQL ASK query is successful");
is($response->content_type, "application/sparql-results+json", "SPARQL ASK query Content Type data is correct");
like($response->content, qr["boolean" : true], "SPARQL ASK Query contains right content");
is_wellformed_json($response->content, "SPARQL ASK query response is valid JSON");

# Test a CONSTRUCT query with a JSON response
$response = $ua->get($base_url."query?query=CONSTRUCT+%7B%3Fs+%3Fp+%3Fo%7D+WHERE+%7B%3Fs+%3Fp+%3Fo%7D&format=json");
is($response->code, 200, "SPARQL CONSTRUCT query for JSON is successful");
is($response->content_type, "application/sparql-results+json", "SPARQL CONSTRUCT query Content Type for JSON is correct");
like($response->content, qr["value" : "v"], "SPARQL CONSTRUCT query for JSON contains right content");
is_wellformed_json($response->content, "SPARQL CONSTRUCT query response is valid JSON");

# Test a CONSTRUCT query with a ntriples response
$response = $ua->get($base_url."query?query=CONSTRUCT+%7B%3Fs+%3Fp+%3Fo%7D+WHERE+%7B%3Fs+%3Fp+%3Fo%7D&format=ntriples");
is($response->code, 200, "SPARQL CONSTRUCT query for ntriples is successful");
is($response->content_type, "text/plain", "SPARQL CONSTRUCT query Content Type for ntriples is correct");
like($response->content, qr[<http://example.org/dir/file#frag>\s+<http://example.org/value>\s+"v"\s+.\n], "SPARQL Construct response for ntriples should be correct");

# Test a SELECT query with a plain text table response
$response = $ua->get($base_url."query?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D%0D%0A&format=table");
is($response->code, 200, "SPARQL SELECT query for plain text table is successful");
is($response->content_type, 'text/plain', "SPARQL SELECT query for plain text Content Type is correct");
like($response->content, qr[string\("v"\)], "SPARQL SELECT Query for plain text contains right content");

# Test content negotiation
{
  $response = $ua->get($base_url.'query?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D%0D%0A', 'Accept' => 'application/sparql-results+json');
  is($response->code, 200, "SPARQL SELECT with content negotiation for JSON is successful");
  is($response->content_type, 'application/sparql-results+json', "SPARQL SELECT with content negotiation for JSON has correct content type");

  $response = $ua->get($base_url.'query?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D%0D%0A', 'Accept' => 'application/sparql-results+xml');
  is($response->code, 200, "SPARQL SELECT with content negotiation for XML is successful");
  is($response->content_type, 'application/sparql-results+xml', "SPARQL SELECT with content negotiation for XML has correct content type");
  
  # Test default mime type
  $response = $ua->get($base_url.'query?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D%0D%0A', 'Accept' => '*/*');
  is($response->code, 200, "SPARQL SELECT with content negotiation for default is successful");
  is($response->content_type, 'application/sparql-results+xml', "SPARQL SELECT with content negotiation for default has correct content type");
}

# Load some more data into the store
load_fixture('foaf.ttl', $base_url.'data/foaf.rdf');

# Test a SELECT query with a LIMIT and an XML response
$response = $ua->get($base_url."query?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D+LIMIT+2&format=xml");
is($response->code, 200, "SPARQL SELECT query with LIMIT is successful");
is($response->content_type, "application/sparql-results+xml", "SPARQL SELECT query with LIMIT Content Type data is correct");
is(scalar(@_ = split(/<result>/,$response->content))-1, 2, "SPARQL SELECT Query Result count is correct");
is_wellformed_xml($response->content, "SPARQL response is valid XML");


# Test alternative SPARQL endpoint URLs
{
    $response = $ua->get($base_url."sparql?query=ASK+%7B%3Fs+%3Fp+%3Fo%7D");
    is($response->code, 200, "GET response for query to /sparql is successful");

    $response = $ua->post($base_url."sparql", {'query' => 'ASK {?s ?p ?o}'});
    is($response->code, 200, "POST response for query to /sparql is successful");

    $response = $ua->get($base_url."sparql/?query=ASK+%7B%3Fs+%3Fp+%3Fo%7D&format=xml");
    is($response->code, 200, "GET response for query to /sparql/ is successful");

    $response = $ua->get($base_url."sparql");
    is($response->code, 400, "GET response to /sparql without query is bad request");

    $response = $ua->post($base_url."sparql");
    is($response->code, 400, "POST response to /sparql without query is bad request");
}



END {
    stop_redstore($pid);
}


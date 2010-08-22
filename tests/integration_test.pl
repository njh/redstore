#!/usr/bin/perl

use POSIX ":sys_wait_h";
use File::Basename qw(dirname);
use LWP::UserAgent;
use HTTP::Response;
use Cwd;
use Errno;
use warnings;
use strict;

use Test::More tests => 154;

my $TEST_CASE_URI = 'http://www.w3.org/2000/10/rdf-tests/rdfcore/xmlbase/test001.rdf';
my $ESCAPED_TEST_CASE_URI = 'http%3A%2F%2Fwww.w3.org%2F2000%2F10%2Frdf-tests%2Frdfcore%2Fxmlbase%2Ftest001.rdf';
my $FOAF_URI = 'http://www.example.com/joe/foaf.rdf';
my $ESCAPED_FOAF_URI = 'http%3A%2F%2Fwww.example.com%2Fjoe%2Ffoaf.rdf';

my $REDSTORE = $ENV{REDSTORE};
$REDSTORE = $ARGV[0] if ($ARGV[0]);
die "Error: REDSTORE environment variable is not set and no argument given" unless ($REDSTORE);
die "Error: redstore command not found: $REDSTORE" unless (-e $REDSTORE);

# Create a libwww-perl user agent
my ($request, $response, @lines);
my $ua = LWP::UserAgent->new;
$ua->timeout(30);
$ua->max_redirect(0);
$ua->default_header('Accept' => "*/*");
$ua->agent('RedStoreTester/1 ');

# Test the usage message
my $usage = `$REDSTORE -h`;
like($usage, qr[Usage: redstore], "The usage message is returned with the -h option");
like($usage, qr[memory\s+In memory lists], "A list of storage types is shown");

# Start RedStore
my ($pid, $port) = start_redstore($REDSTORE);
my $base_url = "http://localhost:$port/";

# Check that the server is running
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

# Test getting Service Description page
$response = $ua->get($base_url.'description', 'Accept' => 'text/html');
is($response->code, 200, "Getting Service Description page is successful");
is($response->content_type, 'text/html', "Service Description page is of type text/html");
ok($response->content_length > 100, "Service Description page is more than 100 bytes long");
is_valid_xhtml($response->content, "Service Description page should be valid XHTML");

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
like($response->content_type, qr[^(application|text)/json$], "SPARQL ASK query Content Type data is correct");
like($response->content, qr["boolean" : false], "SPARQL ASK Query contains right content");
is_wellformed_json($response->content, "SPARQL ASK query response is valid JSON");

# Test POSTing a graph
$request = HTTP::Request->new( 'POST', $base_url.'data/'.$ESCAPED_TEST_CASE_URI );
$request->content( read_fixture('test001.rdf') );
$request->content_length( length($request->content) );
$request->content_type( 'application/rdf+xml' );
$response = $ua->request($request);
is($response->code, 200, "POSTting a data into a graph is successful");
is($response->content_type, 'text/plain', "Non negotiated POST response is of type text/plain");
like($response->content, qr/Successfully added triples/, "Load response message contain triple count");

# Test getting a graph as Turtle
$request = HTTP::Request->new( 'GET', $base_url.'data/'.$ESCAPED_TEST_CASE_URI );
$request->header('Accept', 'application/x-turtle');
$response = $ua->request($request);
is($response->code, 200, "Getting a graph is successful");
is($response->content_type, "application/x-turtle", "Content Type data is correct");
like($response->content, qr[<http://example.org/dir/file#frag>\s+<http://example.org/value>\s+\"v\"\s*.\n], "Graph data is correct");

# Test getting a graph as ntriples
$request = HTTP::Request->new( 'GET', $base_url.'data/'.$ESCAPED_TEST_CASE_URI );
$request->header('Accept', 'text/plain');
$response = $ua->request($request);
is($response->code, 200, "Getting a graph is successful");
@lines = split(/[\r\n]+/, $response->content);
is(scalar(@lines), 1, "Number of triples is correct");

# Test getting list of graphs
$response = $ua->get($base_url.'graphs');
is($response->code, 200, "Getting list of graphs is successful");
is($response->content_type, 'text/plain', "Graph list is of type text/plain");
is($response->content, "$TEST_CASE_URI\n", "List of graphs page contains graph that was added");

# Test getting list of graphs as HTML
$response = $ua->get($base_url.'graphs', 'Accept' => 'text/html');
is($response->code, 200, "Getting list of graphs is successful");
is($response->content_type, 'text/html', "Graph list is of type text/html");
like($response->content, qr[<li><a href="/data/$ESCAPED_TEST_CASE_URI">$TEST_CASE_URI</a></li>], "List of graphs page contains graph that was added");
is_valid_xhtml($response->content, "Graph list should be valid XHTML");

# Test getting list of graphs using SPARQL
$response = $ua->get($base_url."query?query=SELECT+DISTINCT+%3Fg+WHERE+%7BGRAPH+%3Fg+%7B+%3Fs+%3Fp+%3Fo+%7D%7D&format=csv");
is($response->code, 200, "Getting list of graphs using SPARQL is successful");
is($response->content_type, 'text/csv', "Graph list is of type text/csv");
@lines = split(/[\r\n]+/,$response->content);
is(scalar(@lines), 2, "SPARQL response contains two lines");
is($lines[0], "Result,g", "First line of SPARQL response contains CSV header");
is($lines[1], "1,uri($TEST_CASE_URI)", "Second line of SPARQL response contains graph URI");

# Test getting a non-existant graph
$response = $ua->get($base_url."data/http%3A%2F%2Fwww.example.com%2Finvalid");
is($response->code, 404, "Getting a non-existant graph returns 404");
is($response->content_type, 'text/html', "Graph not found page is of type text/html");
ok($response->content_length > 100, "Graph not found page is more than 100 bytes long");
is_valid_xhtml($response->content, "Graph not found page should be valid XHTML");

# Test removing trailing slash
$response = $ua->get($base_url."query/");
is($response->code, 301, "Getting a URL with a trailing slash returns 301");
is($response->header('Location'), "/query", "Getting a URL with a trailing slash returns location without trailing slash");
is_valid_xhtml($response->content, "Redirect page should be valid XHTML");

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
like($response->content_type, qr[^(application|text)/json$], "SPARQL SELECT query Content Type data is correct");
like($response->content, qr[{ "type": "literal", "value": "v" }], "SPARQL SELECT Query contains right content");
is_wellformed_json($response->content, "SPARQL SELECT query response is valid JSON");

# Test a ASK query with a JSON response
$response = $ua->get($base_url."query?query=ASK+%7B%3Fs+%3Fp+%3Fo%7D&format=json");
is($response->code, 200, "SPARQL ASK query is successful");
like($response->content_type, qr[^(application|text)/json$], "SPARQL ASK query Content Type data is correct");
like($response->content, qr["boolean" : true], "SPARQL ASK Query contains right content");
is_wellformed_json($response->content, "SPARQL ASK query response is valid JSON");

# Test a CONSTRUCT query with a ntriples response
$response = $ua->get($base_url."query?query=CONSTRUCT+%7B%3Fs+%3Fp+%3Fo%7D+WHERE+%7B%3Fs+%3Fp+%3Fo%7D&format=ntriples");
is($response->code, 200, "SPARQL CONSTRUCT query is successful");
is($response->content_type, "text/plain", "SPARQL CONSTRUCT query Content Type data is correct");
like($response->content, qr[<http://example.org/dir/file#frag>\s+<http://example.org/value>\s+"v"\s+.\n], "SPARQL Construct response should be correct");

# Test POSTing some Turtle
$request = HTTP::Request->new( 'POST', $base_url.'data/'.$ESCAPED_FOAF_URI );
$request->content( read_fixture('foaf.ttl') );
$request->content_length( length($request->content) );
$request->content_type( 'application/x-turtle' );
$response = $ua->request($request);
is($response->code, 200, "POSTting Tutle formatted data into a graph");
is($response->content_type, 'text/plain', "Non negotiated POST response is of type text/plain");
like($response->content, qr/Successfully added triples/, "Load response message contain triple count");

# Test POSTing some Turtle with an HTML response
$request = HTTP::Request->new( 'POST', $base_url.'data/'.$ESCAPED_FOAF_URI );
$request->content( read_fixture('foaf.ttl') );
$request->content_length( length($request->content) );
$request->content_type( 'application/x-turtle' );
$request->header( 'Accept', 'text/html' );
$response = $ua->request($request);
is($response->code, 200, "POSTting Tutle formatted data into a graph");
is($response->content_type, 'text/html', "POST with HTML response is of type text/html");
like($response->content, qr/Successfully added triples/, "Load response message contain triple count");
is_valid_xhtml($response->content, "HTML Response to POSTing a graph should be valid XHTML");

# Test a SELECT query with a LIMIT and an XML response
$response = $ua->get($base_url."query?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D+LIMIT+2&format=xml");
is($response->code, 200, "SPARQL SELECT query with LIMIT is successful");
is($response->content_type, "application/sparql-results+xml", "SPARQL SELECT query with LIMIT Content Type data is correct");
is(scalar(@_ = split(/<result>/,$response->content))-1, 2, "SPARQL SELECT Query Result count is correct");
is_wellformed_xml($response->content, "SPARQL response is valid XML");

# Test a head request
$response = $ua->head($base_url.'data/'.$ESCAPED_FOAF_URI);
is($response->code, 200, "HEAD response for graph we just created is 200");

# Test DELETEing a graph
$request = HTTP::Request->new( 'DELETE', $base_url.'data/'.$ESCAPED_FOAF_URI );
$response = $ua->request($request);
is($response->code, 200, "DELETEing a graph is successful");

# Test a head request on deleted graph
$response = $ua->head($base_url.'data/'.$ESCAPED_FOAF_URI);
is($response->code, 404, "HEAD response for deleted graph is 404");

# Test dumping the triplestore as N-Quads
$response = $ua->get($base_url."data?format=nquads");
is($response->code, 200, "Triplestore dump successful");
is($response->content_type, 'text/x-nquads', "Triplestore dump is correct MIME type");
@lines = split(/[\r\n]+/, $response->content);
like($lines[-1], qr[^(\S+) (\S+) (\S+) (\S+) \.$], "Last line of dump looks like a quad");

# Test POSTing a url to be loaded
{
    $ua->request(HTTP::Request->new( 'DELETE', $base_url.'data/'.$ESCAPED_FOAF_URI ));
    
    $response = $ua->post( $base_url.'load', {'uri' => fixture_url('foaf.ttl'), 'graph' => $FOAF_URI});
    is($response->code, 200, "POSTing URL to load is successful");
    is($response->content_type, 'text/plain', "Non negotiated load response is of type text/plain");
    like($response->content, qr/Successfully added triples/, "Load response message contain triple count");
    
    # Count the number of triples
    $response = $ua->get($base_url.'data/'.$ESCAPED_FOAF_URI, 'Accept' => 'text/plain');
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 14, "Number of triples in loaded graph is correct");
}

# Test POSTing to /load without a uri
$response = $ua->post( $base_url.'load');
is($response->code, 400, "POSTing to /load without a URI should fail");
like($response->content, qr/Missing URI to load/, "Response mentions missing URI");

# Test POSTing to /load without an empty uri argument
$response = $ua->post( $base_url.'load', {'uri' => ''});
is($response->code, 400, "POSTing to /load an empty 'uri' argument should fail");
like($response->content, qr/Missing URI to load/, "Response mentions missing URI");

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

# Test replacing data with a PUT request
{
    $request = HTTP::Request->new( 'PUT', $base_url.'data/'.$ESCAPED_FOAF_URI );
    $request->content( read_fixture('foaf.ttl') );
    $request->content_length( length($request->content) );
    $request->content_type( 'application/x-turtle' );
    $response = $ua->request($request);
    is($response->code, 200, "Putting data into a graph is successful");
    
    # Count the number of triples
    $response = $ua->get($base_url.'data/'.$ESCAPED_FOAF_URI, 'Accept' => 'text/plain');
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 14, "Number of triples is correct");
    
    # Test PUTing data into a graph
    $request = HTTP::Request->new( 'PUT', $base_url.'data/'.$ESCAPED_FOAF_URI );
    $request->content( read_fixture('test001.rdf') );
    $request->content_length( length($request->content) );
    $request->content_type( 'application/rdf+xml' );
    $response = $ua->request($request);
    is($response->code, 200, "Replacing data in a graph is successful");
    
    # Count the number of triples
    $response = $ua->get($base_url.'data/'.$ESCAPED_FOAF_URI, 'Accept' => 'text/plain');
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 1, "New number of triples is correct");
};

# Test POSTing data into the default graph
{
    $request = HTTP::Request->new( 'POST', $base_url.'data' );
    $request->content( read_fixture('foaf.nt') );
    $request->content_length( length($request->content) );
    $request->content_type( 'text/plain' );
    $response = $ua->request($request);
    is($response->code, 200, "POSTing data into the default graph is succcessful");
    like($response->content, qr/Successfully added triples to the default graph/, "Response messages is correct");

    ## FIXME: check that the number of triples in the store is correct.
}

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
    $response = $ua->get($base_url.'data/test%3Ag', 'Accept' => 'text/plain');
    is($response->code, 200, "Getting the number of triples is successful");
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 2, "New number of triples is correct");
}

# Test POSTing to /insert without a content argument
$response = $ua->post( $base_url.'insert', {'graph' => $FOAF_URI});
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
    $response = $ua->get($base_url.'data/test%3Ag', 'Accept' => 'text/plain');
    is($response->code, 200, "Getting the number of triples is successful");
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 1, "Number of remaining triples is correct");
}

# Test POSTing to /delete without a content argument
$response = $ua->post( $base_url.'delete');
is($response->code, 400, "POSTing to /delete without any content should fail");
like($response->content, qr/Missing the 'content' argument/, "Response mentions missing content argument");

# Test DELETEing everything in the triplestore
{
    $request = HTTP::Request->new( 'DELETE', $base_url.'data' );
    $response = $ua->request($request);
    is($response->code, 200, "DELETEing all the triples is successful");

    # Count the number of triples
    $response = $ua->get($base_url.'data', 'Accept' => 'text/plain');
    is($response->code, 200, "Getting all the triples is successful");
    @lines = split(/[\r\n]+/, $response->content);
    is(scalar(@lines), 0, "There should be no triples remaining");
}



END {
    stop_redstore($pid);
}





sub start_redstore {
    my ($cmd) = @_;
    my ($pid, $port);
    my $count = 0;
    
    unless (-e $cmd) {
        die "Error: RedStore command does not exist: $cmd";
    }
    
    do {
        $port = 10000 + (int(rand(10000) + time()) % 10000);
        $count += 1;

        print "# Starting RedStore on port $port.\n";
        if (!defined($pid = fork())) {
            # fork returned undef, so failed
            die "cannot fork: $!";
        } elsif ($pid == 0) {
            exec("$cmd -q -n -p $port -b localhost -s memory");
            die "failed to exec redstore: $!";
        } 

        sleep(1);
    } while (kill(0,$pid) <= 0 && $count < 10);

    return ($pid, $port);
}

sub stop_redstore {
    my ($pid) = @_;
    my $count = 0;
    
    return unless ($pid);

    while (waitpid($pid,WNOHANG) == 0 && $count < 10) {
        $count += 1;
        if ($count<5) {
            kill(2, $pid);
        } else {
            warn "Sending KILL to $pid\n";
            kill(9, $pid);
        }
        sleep(1);
    }
}

sub is_running {
    my ($pid) = @_;
    ok(kill(0,$pid) > 0, "Process $pid running");
}

sub fixture_path {
    return Cwd::realpath(dirname(__FILE__).'/'.$_[0]);
}

sub fixture_url {
    return 'file://'.fixture_path($_[0]);
}

sub read_fixture {
    my ($filename) = @_;
    my $data = '';
    open(FILE, fixture_path($filename)) or die "Failed to open file: $!";
    while(<FILE>) { $data .= $_; }
    close(FILE);
    return $data;
}

sub is_valid_xhtml {
    my ($xhtml, $name) = @_;
    
    is_wellformed_xml($xhtml, $name);
    
    # FIXME: don't let xmllint download DTDs
    # FIXME: hide "- validates" message
# 	SKIP: {
#         # Check that xmllint is installed
#         if (system("which xmllint > /dev/null")) {
#             skip("xmllint is not available", 1);
#         }
#     
#         # Open a pipe to xmllint
#         open(LINT, "|xmllint --noout --valid --schema xhtml1-strict.xsd -")
#             or die "Failed to open pipe to xmllint: $!";
#         print LINT $xhtml;
#         close(LINT);
# 
#         if ($?) {
#         	fail($name);
#         } else {
#         	pass($name);
#         }
#     }
}

sub is_wellformed_xml {
    my ($xml, $name) = @_;

	SKIP: {
		eval { require XML::Parser; };
		skip("XML::Parser not installed", 1) if $@;

		my $parser = new XML::Parser(ErrorContext => 2);
    	eval { $parser->parse($xml); };
    	if ($@) {
        	$@ =~ s/at \/.*?$//s;
        	print STDERR "$@\n";
        	fail($name);
    	} else {
        	pass($name);
    	}
    }
}

sub is_wellformed_json {
    my ($json, $name) = @_;

	SKIP: {
		eval { require JSON ; };
		skip("JSON module not installed", 1) if $@;
		skip("JSON module is too old", 1) if (!defined $JSON::VERSION or $JSON::VERSION < 2.0);

    	eval { JSON::from_json($json); };
    	if ($@) {
        	$@ =~ s/at \/.*?$//s;
        	print STDERR "$@\n";
        	fail($name);
    	} else {
        	pass($name);
    	}
    }
}


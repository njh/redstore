#!/usr/bin/perl

use POSIX ":sys_wait_h";
use File::Basename qw(dirname);
use LWP::UserAgent;
use HTTP::Response;
use Errno;
use warnings;
use strict;

use Test::More tests => 35;

my $REDSTORE = dirname(__FILE__) . '/../src/redstore';
my $TEST_CASE_URI = 'http://www.w3.org/2000/10/rdf-tests/rdfcore/xmlbase/test001.rdf';
my $ESCAPED_TEST_CASE_URI = 'http%3A%2F%2Fwww.w3.org%2F2000%2F10%2Frdf-tests%2Frdfcore%2Fxmlbase%2Ftest001.rdf';

# Create a libwww-perl user agent
my ($request, $response);
my $ua = LWP::UserAgent->new;
$ua->timeout(10);
$ua->max_redirect(0);
$ua->default_header('Accept' => "*/*");
$ua->agent('RedStoreTester/1 ');


# Start RedStore
my ($pid, $port) = start_redstore($REDSTORE);
my $base_url = "http://localhost:$port/";

# Check that the server is running
is_running($pid);

# Test getting the homepage
$response = $ua->get($base_url);
is($response->code, 200, "Getting homepage is successful");
is_wellformed($response->content, "Homepage is valid XML");

# Test getting the query page
$response = $ua->get($base_url.'query');
is($response->code, 200, "Getting query page is successful");
is_wellformed($response->content, "Query page is valid XML");

# Test getting information page
$response = $ua->get($base_url.'info');
is($response->code, 200, "Getting info page is successful");
is_wellformed($response->content, "Info page is valid XML");

# Test getting formats page
$response = $ua->get($base_url.'formats');
is($response->code, 200, "Getting formats page is successful");
is_wellformed($response->content, "Formats page is valid XML");

# Test getting load page
$response = $ua->get($base_url.'load');
is($response->code, 200, "Getting load page is successful");
is_wellformed($response->content, "Load page is valid XML");

# Test getting the favicon
$response = $ua->get($base_url.'favicon.ico');
is($response->code, 200, "Getting favicon.ico is successful");
is($response->content_type, 'image/x-icon', "Getting favicon.ico set the content type");
is($response->content_length, 318, "Getting favicon.ico set the content length header");

# Test getting empty list of graphs
$response = $ua->get($base_url.'data');
is($response->code, 200, "Getting empty list of graphs is successful");
like($response->content, qr[<ul>\n</ul>], "List of graphs page contains empty unordered list");
is_wellformed($response->content, "Empty list of graphs is valid XML");

# Test PUTing a graph
$request = HTTP::Request->new( 'PUT', $base_url.'data/'.$ESCAPED_TEST_CASE_URI );
$request->content( read_fixture('test001.rdf') );
$request->content_length( length($request->content) );
$request->content_type( 'application/rdf+xml' );
$response = $ua->request($request);
is($response->code, 200, "Putting a data into a graph");
like($response->content, qr/Added 1 triples to graph/, "Load response message contain triple count");
is_wellformed($response->content, "Response to putting a graph is valid XML");

# Test getting a graph
$request = HTTP::Request->new( 'GET', $base_url.'data/'.$ESCAPED_TEST_CASE_URI );
$request->header('Accept', 'application/x-ntriples');
$response = $ua->request($request);
is($response->code, 200, "Getting a graph is successful");
is($response->content_type, "application/x-ntriples", "Content Type data is correct");
is($response->content, "<http://example.org/dir/file#frag> <http://example.org/value> \"v\" .\n", "Graph data is correct");

# Test getting list of graphs
$response = $ua->get($base_url.'data');
is($response->code, 200, "Getting list of graphs is successful");
like($response->content, qr[<li><a href="/data/$ESCAPED_TEST_CASE_URI">$TEST_CASE_URI</a></li>], "List of graphs page contains graph that was added");
is_wellformed($response->content, "Graph list is valid XML");

# Test getting a non-existant graph
$response = $ua->get($base_url."data/http%3A%2F%2Fwww.example.com%2Finvalid");
is($response->code, 404, "Getting a non-existant graph returns 404");
is_wellformed($response->content, "Graph not found page is valid XML");

# Test removing trailing slash
$response = $ua->get($base_url."query/");
is($response->code, 301, "Getting a URL with a trailing slash returns 301");
is($response->header('Location'), "/query", "Getting a URL with a trailing slash returns location without trailing slash");
is_wellformed($response->content, "Redirect page is valid XML");

# Test a SELECT query
$response = $ua->get($base_url."sparql?query=SELECT+*+WHERE+%7B%3Fs+%3Fp+%3Fo%7D%0D%0A&format=xml");
is($response->code, 200, "SPARQL SELECT query is successful");
is($response->content_type, "application/sparql-results+xml", "SPARQL SELECT query Content Type data is correct");
like($response->content, qr[<binding name="o"><literal>v</literal></binding>], "SPARQL SELECT Query contains right content");
is_wellformed($response->content, "SPARQL response is valid XML");

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
    ok(kill(0,$pid) > 0, "Process $pid running.");
}

sub read_fixture {
    my ($filename) = @_;
    my $data = '';
    open(FILE, dirname(__FILE__).'/'.$_[0]) or die "Failed to open file: $!";
    while(<FILE>) { $data .= $_; }
    close(FILE);
    return $data;
}

sub is_wellformed {
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


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


use Test::More tests => 29;

my $rdf_ns = 'http://www.w3.org/1999/02/22-rdf-syntax-ns';
my $sd_ns = 'http://www.w3.org/ns/sparql-service-description';
my $void_ns = 'http://rdfs.org/ns/void';
my $xsd_ns = 'http://www.w3.org/2001/XMLSchema';
my $rdfs_ns = 'http://www.w3.org/2000/01/rdf-schema';
my $format_ns = 'http://www.w3.org/ns/formats/';


# Create a libwww-perl user agent
my ($request, $response, @lines);
my $ua = new_redstore_client();

# Start RedStore
my ($pid, $base_url) = start_redstore();

# Double check that the server is running
is_running($pid);

# Load some test data into the store
load_fixture('foaf.ttl', $base_url.'data/foaf.rdf');

# GET the service description from the SPARQL endpoint using content negotiation
$response = $ua->get($base_url.'sparql', 'Accept' => 'application/rdf+xml');
is($response->code, 200, 'GET to sparql endpoint for application/rdf+xml is successful');
is($response->content_type, "application/rdf+xml", "GET to sparql endpoint for application/rdf+xml has correct MIME type");
like($response->content, qr/<sd:Service>/, "GET to sparql endpoint returns RDF/XML containing '<sd:Service>'");

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

# GET the service description from its own URL
$response = $ua->get($base_url.'description', 'Accept' => 'application/rdf+xml');
is($response->code, 200, 'GET to description endpoint for application/rdf+xml is successful');
is($response->content_type, "application/rdf+xml", "GET to description endpoint for application/rdf+xml has correct MIME type");
like($response->content, qr/<sd:Service>/, "GET to description endpoint returns RDF/XML containing '<sd:Service>'");

# GET the service description from its own URL as n-triples
$response = $ua->get($base_url.'description?format=ntriples');
is($response->code, 200, 'GET to description endpoint for application/rdf+xml is successful');
is($response->content_type, "text/plain", "GET to description endpoint for application/rdf+xml has correct MIME type");

# Check for the service node
my ($service_node) = ($response->content =~ qr[(\S+) <$rdf_ns#type> <$sd_ns#Service>]);
like($service_node, qr[_:\w+], 'There is a bnode of type sd:Service');

# Check for endpoint triple
my ($endpoint_node) = ($response->content =~ qr[$service_node <$sd_ns#endpoint> <(\S+)>]);
is($endpoint_node, $base_url.'sparql', 'There is an sd:enpoint triple');

# Check for the default union graph feature
like(
  $response->content, qr[$service_node <$sd_ns#feature> <$sd_ns#UnionDefaultGraph>],
  'The default union graph feature is set.'
);

# Check for the default dataset
my ($dataset_node) = ($response->content =~ qr[$service_node <$sd_ns#defaultDatasetDescription> (\S+)]);
like($dataset_node, qr[_:\w+], 'There is a default dataset defined for the service');

# Check that the default dataset is of the right type
like(
  $response->content, qr[$dataset_node <$rdf_ns#type> <$sd_ns#Dataset>],
  'The default dataset node is of type sd:Dataset.'
);

# Check for the default graph
my ($graph_node) = ($response->content =~ qr[$dataset_node <$sd_ns#defaultGraph> (\S+)]);
like($graph_node, qr[_:\w+], 'There is a default graph defined for the default dataset');

# Check that the default graph is of the right type
like(
  $response->content, qr[$graph_node <$rdf_ns#type> <$sd_ns#Graph>],
  'The default dataset node is of type sd:Graph.'
);

# Check the number of triples in the default graph
like(
  $response->content, qr[$graph_node <$void_ns#triples> "14"\^\^<$xsd_ns#integer>],
  'The correct number of triples is in the default graph.'
);

# Check for Turtle format
# FIXME: this should lookup by Format URI, once Redland supports it
my ($turtle_node) = ($response->content =~ qr[(\S+) <$rdfs_ns#label> "turtle"]);
like($turtle_node, qr[^<http://], 'There is a Format with label "Turtle" in the description.');
like(
  $response->content, qr[$turtle_node <$rdf_ns#type> <${format_ns}Format>],
  'The turtle node is of type format:Format.'
);
like(
  $response->content, qr[$turtle_node <${format_ns}media_type> "application/turtle"],
  'The turtle node has the MIME type "application/turtle" associated with it.'
);
like(
  $response->content, qr[$service_node <$sd_ns#inputFormat> $turtle_node],
  'The turtle is a supported input format.'
);
like(
  $response->content, qr[$service_node <$sd_ns#resultFormat> $turtle_node],
  'The turtle is a supported result format.'
);


END {
    stop_redstore($pid);
}


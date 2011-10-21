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


use Test::More tests => 30;

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

# GET the service description from the SPARQL endpoint using content negotiation
$response = $ua->get($base_url.'sparql', 'Accept' => 'application/x-turtle');
is($response->code, 200, 'GET to sparql endpoint for application/x-turtle is successful');
is($response->content_type, "application/x-turtle", "GET to sparql endpoint for application/rdf+xml has correct MIME type");
like($response->content, qr/ sd:Service /, "GET to sparql endpoint returns Turle containing 'sd:Service'");

# GET the service description from its own URL
$response = $ua->get($base_url.'description?format=rdfxml-abbrev');
is($response->code, 200, 'GET to description endpoint for rdfxml-abbrev is successful');
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
# my ($turtle_node) = ($response->content =~ qr[(\S+) <$rdfs_ns#comment> "Turtle Terse RDF Triple Language"]);
# like($turtle_node, qr[^<http://], 'There is a Format with comment "Turtle" in the description.');
# like(
#   $response->content, qr[$turtle_node <$rdfs_ns#label> "turtle"],
#   'The turtle node has the label "turtle".'
# );
# like(
#   $response->content, qr[$turtle_node <$rdf_ns#type> <${format_ns}Format>],
#   'The turtle node is of type format:Format.'
# );
# # FIXME: enable this
# # like(
# #   $response->content, qr[$rdfxml_node <$rdfs_ns#isDefinedBy> <http://www.w3.org/TR/rdf-turtle>],
# #   'The turtle node is has a rdfs:isDefinedBy triple.'
# # );
# like(
#   $response->content, qr[$turtle_node <${format_ns}media_type> "application/turtle"],
#   'The turtle node has the MIME type "application/turtle" associated with it.'
# );
# like(
#   $response->content, qr[$service_node <$sd_ns#inputFormat> $turtle_node],
#   'The turtle is a supported input format.'
# );
# like(
#   $response->content, qr[$service_node <$sd_ns#resultFormat> $turtle_node],
#   'The turtle is a supported result format.'
# );


# Check for RDF/XML format
# FIXME: this should lookup by Format URI, once Redland supports it
# my ($rdfxml_node) = ($response->content =~ qr[(\S+) <$rdfs_ns#comment> "RDF/XML"]);
# like($rdfxml_node, qr[^<http://], 'There is a Format with comment "RDF/XML" in the description.');
# like(
#   $response->content, qr[$rdfxml_node <$rdfs_ns#label> "rdfxml"],
#   'The rdfxml node has the label "rdfxml".'
# );
# like(
#   $response->content, qr[$rdfxml_node <$rdf_ns#type> <${format_ns}Format>],
#   'The rdfxml node is of type format:Format.'
# );
# like(
#   $response->content, qr[$rdfxml_node <$rdfs_ns#isDefinedBy> <http://www.w3.org/TR/rdf-syntax-grammar>],
#   'The rdfxml node is has a rdfs:isDefinedBy triple.'
# );
# like(
#   $response->content, qr[$rdfxml_node <${format_ns}media_type> "application/rdf\+xml"],
#   'The rdfxml node has the MIME type "application/rdf+xml" associated with it.'
# );
# like(
#   $response->content, qr[$service_node <$sd_ns#inputFormat> $rdfxml_node],
#   'The rdfxml is a supported input format.'
# );
# like(
#   $response->content, qr[$service_node <$sd_ns#resultFormat> $rdfxml_node],
#   'The rdfxml is a supported result format.'
# );


# Check for SPARQL XML results format
# FIXME: this should lookup by Format URI, once Redland supports it
# my ($xml_node) = ($response->content =~ qr[(\S+) <$rdfs_ns#comment> "SPARQL XML Query Results"]);
# like($xml_node, qr[^<http://], 'There is a Format with comment "SPARQL XML Query Results" in the description.');
# like(
#   $response->content, qr[$xml_node <$rdfs_ns#label> "xml"],
#   'The SPARQL XML node has the label "xml".'
# );
# like(
#   $response->content, qr[$xml_node <$rdf_ns#type> <${format_ns}Format>],
#   'The SPARQL XML node is of type format:Format.'
# );
# like(
#   $response->content, qr[$xml_node <$rdfs_ns#isDefinedBy> <http://www.w3.org/TR/rdf-sparql-XMLres/>],
#   'The SPARQL XML node is has a rdfs:isDefinedBy triple.'
# );
# like(
#   $response->content, qr[$xml_node <${format_ns}media_type> "application/sparql-results\+xml"],
#   'The SPARQL XML node has the MIME type "application/sparql-results+xml" associated with it.'
# );
# like(
#   $response->content, qr[$service_node <$sd_ns#resultFormat> $xml_node],
#   'The SPARQL XML is a supported result format.'
# );


# Check for the GraphViz format (a bnode)
my ($dot_node) = ($response->content =~ qr[(\S+) <$rdfs_ns#comment> "GraphViz DOT format"]);
like($dot_node, qr[^_:], 'There is a Format with comment "GraphViz DOT format" in the description.');
like(
  $response->content, qr[$dot_node <$rdfs_ns#label> "dot"],
  'The GraphViz node has the label "dot".'
);
like(
  $response->content, qr[$dot_node <$rdf_ns#type> <${format_ns}Format>],
  'The GraphViz node is of type format:Format.'
);
like(
  $response->content, qr[$dot_node <$rdfs_ns#isDefinedBy> <http://www.graphviz.org/doc/info/lang.html>],
  'The GraphViz node is has a rdfs:isDefinedBy triple.'
);
like(
  $response->content, qr[$dot_node <${format_ns}media_type> "text/x-graphviz"],
  'The GraphViz node has the MIME type "text/x-graphviz" associated with it.'
);
like(
  $response->content, qr[$service_node <$sd_ns#resultFormat> $dot_node],
  'The GraphViz is a supported result format.'
);


END {
    stop_redstore($pid);
}


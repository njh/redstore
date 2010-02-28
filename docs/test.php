<?php

include_once "Redstore.php";

$redstore = new Redstore('http://localhost:8080/');
$redstore->load('http://www.aelius.com/njh/foaf.rdf') or die("Failed to Load.\n");

$redstore->put(
    '<http://a.com/s> <http://a.com/p> "o" .',
    'http://a.com/foo.rdf', 'application/x-turtle'
) or die("Failed to Put.\n");

$data = $redstore->get('http://a.com/foo.rdf', 'application/x-ntriples');
if (!$redstore->is_success()) { die("Failed to GET.\n"); }
if ($data != "<http://a.com/s> <http://a.com/p> \"o\" .\n") {
    die("Data returned from GET isn't what was expected:\n$data\n");
}

$redstore->delete('http://a.com/foo.rdf') or die("Failed to Delete.\n");

$data = $redstore->get('http://a.com/foo.rdf');
if ($redstore->is_success()) { die("Succeed in getting deleted graph.\n"); }

print "All tests ok.\n";

?>

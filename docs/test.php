<?php

include_once "Redstore.php";


$redstore = new Redstore('http://localhost:8080/');
$redstore->delete('http://www.aelius.com/njh/foaf.rdf');
$redstore->load('http://www.aelius.com/njh/foaf.rdf');


?>

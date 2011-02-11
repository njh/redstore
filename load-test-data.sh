#!/bin/sh

curl -T tests/foaf.ttl -H 'Content-Type: application/x-turtle' 'http://localhost:8080/data/data.rdf'

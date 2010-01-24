#!/bin/sh

URLS="
  http://www.aelius.com/njh/foaf.rdf
  http://moustaki.org/foaf.rdf
  http://metade.org/foaf.rdf
"

for url in $URLS; do
	curl --data uri=$url http://localhost:8080/load
    #rdfproc -c redstore parse $url rdfxml $url $url
done

#rdfproc -c redstore contexts
#rdfproc -c redstore size

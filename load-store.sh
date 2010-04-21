#!/bin/sh

URLS="
  http://www.aelius.com/njh/foaf.rdf
  http://moustaki.org/foaf.rdf
  http://metade.org/foaf.rdf
"

for url in $URLS; do
  #rdfproc -c redstore parse $url rdfxml $url $url
	curl --data uri=$url http://localhost:8080/load
done

echo "List of named graphs:"
#rdfproc -c redstore contexts
curl http://localhost:8080/graphs

#rdfproc -c redstore size
#curl http://localhost:8080/description/totalTriples

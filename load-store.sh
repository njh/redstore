#!/bin/sh

rm -f redstore-contexts.db	redstore-po2s.db redstore-so2p.db redstore-sp2o.db

URLS="
  http://www.aelius.com/njh/foaf.rdf
  http://moustaki.org/foaf.rdf
  http://metade.org/foaf.rdf
"

for url in $URLS; do
    rdfproc -c redstore parse $url rdfxml $url $url
done

rdfproc -c redstore contexts
rdfproc -c redstore size

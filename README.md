RedStore
========
Nicholas J. Humfrey <njh@aelius.com>

For the latest version of RedStore, please see:
<http://www.aelius.com/njh/redstore/>


What is RedStore ?
------------------
RedStore is a lightweight RDF triplestore written in C using the Redland library.

Features:

* SPARQL over HTTP support
* Compatible with rdfproc command line tool for offline operations
* An HTTP interface that is compatible with 4store.
* Only build dependancy is Redland.
* Unit tests for most of the HTTP server code.


Usage
-----
    redstore [options] [<name>]
       -p <port>       Port number to run HTTP server on (default 8080)
       -b <address>    Bind to specific address (default all)
       -s <type>       Set the graph storage type
       -t <options>    Storage options
       -n              Create a new store / replace old (default no)
       -v              Enable verbose mode
       -q              Enable quiet mode
  

Add a URI to the triplestore:
    curl --data uri=http://example.com/file.rdf http://localhost:8080/load

Add a file to the triplestore:
    curl -T file.rdf 'http://localhost:8080/data/http://example.com/data'

Add a file to the triplestore with type specified:
    curl -T file.rdf -H 'Content-Type: application/x-turtle' 'http://localhost:8080/data/data.rdf'
 
You can delete graphs with in the same manner, using the DELETE HTTP verb:
    curl -X DELETE 'http://localhost:8080/data/http://example.com/data'

Query using the [SPARQL Query Tool]:
    sparql-query http://localhost:8080/sparql 'SELECT * WHERE { ?s ?p ?o } LIMIT 10'


Requirements
------------

The following versions of Redland are recommend:

- [raptor2-2.0.0]
- [rasqal-0.9.24]
- [redland-1.0.13]


Installation
------------
RedStore uses a standard automake build process:

    ./configure
    make
    make install


Supported Storage Modules
-------------------------

You can use any of the [Redland Storage Modules] that supports contexts:

- [hashes]
- [mysql]
- [memory] (Default)
- [postgresql]
- [sqlite]
- [virtuoso]


License
-------

This program is free software: you can redistribute it and/or modify
it under the terms of the [GNU General Public License] as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.



[Redland Storage Modules]:     http://librdf.org/docs/api/redland-storage-modules.html
[SPARQL Query Tool]:           http://github.com/tialaramex/sparql-query
[GNU General Public License]:  http://www.gnu.org/licenses/gpl.html

[raptor2-2.0.0]:               http://download.librdf.org/source/raptor2-2.0.0.tar.gz
[rasqal-0.9.24]:               http://download.librdf.org/source/rasqal-0.9.24.tar.gz
[redland-1.0.13]:              http://download.librdf.org/source/redland-1.0.13.tar.gz

[hashes]:                      http://librdf.org/docs/api/redland-storage-module-hashes.html
[mysql]:                       http://librdf.org/docs/api/redland-storage-module-mysql.html
[memory]:                      http://librdf.org/docs/api/redland-storage-module-memory.html
[postgresql]:                  http://librdf.org/docs/api/redland-storage-module-postgresql.html
[sqlite]:                      http://librdf.org/docs/api/redland-storage-module-sqlite.html
[virtuoso]:                    http://librdf.org/docs/api/redland-storage-module-virtuoso.html

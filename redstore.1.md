% REDSTORE(1)
% Nicholas J Humfrey
% October 26, 2011

NAME
====
**redstore** - lightweight RDF triplestore with HTTP interface


SYNOPSIS
========
**redstore** [*options*] [*name*]


DESCRIPTION
===========
RedStore is a lightweight RDF triplestore powered by the Redland library. 

It has a HTTP interface and supports the following W3C standards:

* [SPARQL 1.0 Query]
* [SPARQL 1.1 Protocol for RDF]
* [SPARQL 1.1 Graph Store HTTP Protocol]
* [SPARQL 1.1 Service Description]


OPTIONS
=======
`-p` *port*
:   The HTTP port number that the built-in HTTP server will listen on.
    By default, RedStore will listen on port 8080.

`-b` *address*
:   The IP address of the network interface to listen on.
    By default, RedStore will listen on all network interfaces.
    This may be a security risk - use `-b localhost` to make RedStore
    only available on your local machine.        

`-s` *type*
:   Set the graph storage type.
    By default, RedStore uses the 'hashes' storage type.
    You can use any of the storage modules that support contexts.

`-t` *options*
:   Select storage options for the chosen storage type.
    See the [Redland storage documentation] for details of the 
    available options for each storage type.
    Contexts and write-mode are enabled by default in RedStore.

`-n`
:   Create a new store / replace old.
    This sets the *new=yes* storage option.
    This may be required when you first create a new store or
    want to delete the contents of an existing store.

`-f` *filename*
:   Select an input file to load at startup. This file will loaded
    into the default graph at startup. Combined with the `-n` option it
    may be useful to restore your store to a known state.

`-F` *format*
:   Specifies the format of the input file.
    The default is to attempt to guess the storage type.

`-v`
:   Enable verbose mode - display debugging messages in the log.

`-q`
:   Enable quiet mode - disables information and debugging messages.
    Warning and error messages are still enabled.


EXIT STATUS
===========
RedStore exits 0 on success, and >0 if an error occurred that caused RedStore to exit.


RESOURCES
=========
[RedStore Homepage]

[RedStore on GitHub]

[Redland Homepage]


AUTHORS
=======
RedStore was written by Nicholas Humfrey.


BUGS
====
Bugs are tracked on the GitHub issue tracker:
  <http://github.com/njh/redstore/issues>


COPYING
=======
Copyright \(C) 2010-2011 Nicholas J Humfrey. Free use of this software is
granted under the terms of the GNU General Public License (GPL).


[SPARQL 1.0 Query]:                     http://www.w3.org/TR/rdf-sparql-query/
[SPARQL 1.1 Protocol for RDF]:          http://www.w3.org/TR/sparql11-protocol/
[SPARQL 1.1 Graph Store HTTP Protocol]: http://www.w3.org/TR/sparql11-http-rdf-update/
[SPARQL 1.1 Service Description]:       http://www.w3.org/TR/sparql11-service-description/
[Redland storage documentation]:        http://librdf.org/docs/api/redland-storage-modules.html

[RedStore Homepage]:                    http://www.aelius.com/njh/redstore/
[RedStore on GitHub]:                   http://github.com/njh/redstore
[Redland Homepage]:                     http://librdf.org/

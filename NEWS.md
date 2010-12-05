News about RedStore releases
============================

Release 0.5.1 - 2010-12-05
--------------------------

- Redstore now depends upon Raptor 2.
- Displays storage options configuration at startup
- Enabled support for parsing of JSON (using Raptor 2)
- Changes so that content negotation takes account of the q values from raptor.
- Removed Redstore's built-in HTML serialiser in favour of using Rasqal/Raptor.


Release 0.4 - 2010-04-22
------------------------

- Added support for inserting/deleting triples via HTTP POST from a form.
- Implemented content negotiation for DELETE response.
- Added support for POSTing triples into the default graph.


Release 0.3 - 2010-04-11
------------------------

- Added improved content negotiation support
- Added HTML entity escaping
- Created new Service Description page
- Full format list on query page
- Added support for selecting query language
- Added support for ASK queries
- text/plain load messages unless HTML is requested
- N-quads dumping support


Release 0.2 - 2010-02-14
------------------------

- Fixed invalid XHTML
- Fixed bug in content buffering
- Changed error pages to XHTML
- Changes to fix 'make distcheck'
- Added check for checkmk command
- More tests
- Now using libtool to generate convenience library


Release 0.1 - 2010-02-12
------------------------

- Initial Release.

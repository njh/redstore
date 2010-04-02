/*
    RedStore - a lightweight RDF triplestore powered by Redland
    Copyright (C) 2010 Nicholas J Humfrey <njh@aelius.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "redstore.h"


redhttp_response_t *redstore_error_page(int level, int code, const char *message)
{
    redstore_log(level, message);
    return redhttp_response_new_error_page(code, message);
}

void page_append_html_header(redhttp_response_t * response, const char *title)
{
    redhttp_response_add_header(response, "Content-Type", "text/html");
    redhttp_response_content_append(response,
                                    "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                                    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
                                    "          \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
                                    "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
                                    "<head>\n"
                                    "  <title>RedStore - %s</title>\n"
                                    "</head>\n" "<body>\n" "<h1>%s</h1>\n", title, title);
}

void page_append_html_footer(redhttp_response_t * response)
{
    redhttp_response_content_append(response,
                                    "<hr /><address>%s librdf/%s raptor/%s rasqal/%s</address>\n"
                                    "</body></html>\n",
                                    PACKAGE_NAME "/" PACKAGE_VERSION,
                                    librdf_version_string,
                                    raptor_version_string, rasqal_version_string);
}

redhttp_response_t *handle_page_home(redhttp_request_t * request, void *user_data)
{
    redhttp_response_t *response = redhttp_response_new(REDHTTP_OK, NULL);
    page_append_html_header(response, "RedStore");
    redhttp_response_content_append(response,
                                    "<ul>\n"
                                    "  <li><a href=\"/query\">SPARQL Query Page</a></li>\n"
                                    "  <li><a href=\"/load\">Load URI</a></li>\n"
                                    "  <li><a href=\"/data\">Named Graphs</a></li>\n"
                                    "  <li><a href=\"/dump\">Download Full Dump</a></li>\n"
                                    "  <li><a href=\"/description\">Service Description</a></li>\n"
                                    "</ul>\n");
    page_append_html_footer(response);
    return response;
}


redhttp_response_t *handle_page_query(redhttp_request_t * request, void *user_data)
{
    redhttp_response_t *response = redhttp_response_new(REDHTTP_OK, NULL);
    page_append_html_header(response, "SPARQL Query");

    redhttp_response_content_append(response,
                                    "<form action=\"../sparql\" method=\"get\">\n"
                                    "<div><textarea name=\"query\" cols=\"80\" rows=\"18\">"
                                    "PREFIX rdf: &lt;http://www.w3.org/1999/02/22-rdf-syntax-ns#&gt;\n"
                                    "PREFIX rdfs: &lt;http://www.w3.org/2000/01/rdf-schema#&gt;\n"
                                    "\n"
                                    "SELECT * WHERE {\n"
                                    " ?s ?p ?o\n"
                                    "} LIMIT 10\n"
                                    "</textarea></div>\n"
                                    "<div class=\"buttons\">Output Format: <select name=\"format\">\n"
                                    "  <option value=\"html\">HTML</option>\n"
                                    "  <option value=\"text\">Plain Text</option>\n"
                                    "  <option value=\"xml\">XML</option>\n"
                                    "  <option value=\"json\">JSON</option>\n"
                                    "</select>\n"
                                    "<input type=\"reset\" /> "
                                    "<input type=\"submit\" value=\"Execute\" /></div>\n</form>\n");

    // FIXME: list output formats based on enumeration of formats that Redland supports

    page_append_html_footer(response);

    return response;
}

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

#include "redstore.h"


static redhttp_response_t *handle_html_graph_index(redhttp_request_t * request,
                                                   librdf_iterator * iterator)
{
    redhttp_response_t *response = redstore_page_new("Named Graphs");
    if (!response)
        return NULL;

    redstore_page_append_string(response, "<ul>\n");

    while (!librdf_iterator_end(iterator)) {
        librdf_uri *uri;
        librdf_node *node;
        char *escaped;

        node = (librdf_node *) librdf_iterator_get_object(iterator);
        if (!node) {
            redstore_error("librdf_iterator_get_next returned NULL");
            break;
        }

        uri = librdf_node_get_uri(node);
        if (!uri) {
            redstore_error("Failed to get URI for context node");
            break;
        }

        escaped = redhttp_url_escape((char *) librdf_uri_as_string(uri));
        redstore_page_append_string(response, "<li><a href=\"/data/");
        redstore_page_append_escaped(response, escaped, 0);
        redstore_page_append_string(response, "\">");
        redstore_page_append_escaped(response, (char *) librdf_uri_as_string(uri), 0);
        redstore_page_append_string(response, "</a></li>\n");
        free(escaped);

        librdf_iterator_next(iterator);
    }
    redstore_page_append_string(response, "</ul>\n");

    redstore_page_append_string(response,
                                "<p>This document is also available as <a href=\"/data?format=text\">plain text</a>.</p>\n");

    redstore_page_end(response);

    return response;
}

static redhttp_response_t *handle_text_graph_index(redhttp_request_t * request,
                                                   librdf_iterator * iterator)
{
    redhttp_response_t *response = redhttp_response_new_with_type(REDHTTP_OK, NULL, "text/plain");
    FILE *socket = redhttp_request_get_socket(request);
    redhttp_response_send(response, request);


    if (!response)
        return NULL;

    while (!librdf_iterator_end(iterator)) {
        librdf_uri *uri;
        librdf_node *node;

        node = (librdf_node *) librdf_iterator_get_object(iterator);
        if (!node) {
            redstore_error("librdf_iterator_get_next returned NULL");
            break;
        }

        uri = librdf_node_get_uri(node);
        if (!uri) {
            redstore_error("Failed to get URI for context node");
            break;
        }

        fprintf(socket, "%s\n", librdf_uri_as_string(uri));

        librdf_iterator_next(iterator);
    }

    return response;
}


redhttp_response_t *handle_graph_index(redhttp_request_t * request, void *user_data)
{
    redhttp_response_t *response = NULL;
    librdf_iterator *iterator = NULL;
    char *format_str = redstore_get_format(request, "text/plain,text/html,application/xhtml+xml");

    iterator = librdf_storage_get_contexts(storage);
    if (!iterator) {
        return redstore_error_page(REDSTORE_ERROR,
                                   REDHTTP_INTERNAL_SERVER_ERROR, "Failed to get list of graphs.");
    }

    if (redstore_is_text_format(format_str)) {
        response = handle_text_graph_index(request, iterator);
    } else if (redstore_is_html_format(format_str)) {
        response = handle_html_graph_index(request, iterator);
    } else {
        response = redstore_error_page(REDSTORE_INFO,
                                       REDHTTP_NOT_ACCEPTABLE, "No acceptable format supported.");
    }

    free(format_str);
    librdf_free_iterator(iterator);

    return response;
}

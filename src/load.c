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


redhttp_response_t *handle_load_get(redhttp_request_t * request, void *user_data)
{
    redhttp_response_t *response = redstore_page_new("Load URI");
    redstore_page_append_string(response, "<form method=\"post\" action=\"/load\"><div>\n"
                                    "<label for=\"uri\">URI:</label> <input id=\"uri\" name=\"uri\" type=\"text\" size=\"40\" /><br />\n"
                                    "<label for=\"base-uri\">Base URI:</label> <input id=\"base-uri\" name=\"base-uri\" type=\"text\" size=\"40\" /> <i>(optional)</i><br />\n"
                                    "<label for=\"graph\">Graph:</label> <input id=\"graph\" name=\"graph\" type=\"text\" size=\"40\" /> <i>(optional)</i><br />\n"
                                    "<input type=\"reset\" /> <input type=\"submit\" />\n"
                                    "</div></form>\n");

    redstore_page_end(response);

    return response;
}

redhttp_response_t *load_stream_into_graph(librdf_stream * stream, librdf_uri * graph_uri)
{
    redhttp_response_t *response = NULL;
    librdf_node *graph = NULL;
    size_t count = 0;

    graph = librdf_new_node_from_uri(world, graph_uri);
    if (!graph) {
        return redstore_error_page(REDSTORE_ERROR,
                                   REDHTTP_INTERNAL_SERVER_ERROR,
                                   "librdf_new_node_from_uri failed for Graph URI");
    }

    while (!librdf_stream_end(stream)) {
        librdf_statement *statement = librdf_stream_get_object(stream);
        if (!statement) {
            response =
                redstore_error_page(REDSTORE_ERROR,
                                    REDHTTP_INTERNAL_SERVER_ERROR,
                                    "Failed to get statement from stream.");
            break;
        }

        if (librdf_model_context_add_statement(model, graph, statement)) {
            response =
                redstore_error_page(REDSTORE_ERROR,
                                    REDHTTP_INTERNAL_SERVER_ERROR,
                                    "Failed to add statement to graph.");
            break;
        }
        count++;
        librdf_stream_next(stream);
    }

    // FIXME: check for parse errors or parse warnings
    if (!response) {
        response = redstore_page_new("Success");
        redstore_info("Added %d triples to graph.", count);
        redstore_page_append_string(response, "<p>Added ");
        redstore_page_append_decimal(response, count);
        redstore_page_append_string(response, " triples to graph: ");
        redstore_page_append_escaped(response, (char*)librdf_uri_as_string(graph_uri), 0);
        redstore_page_append_string(response,"</p>\n");
        redstore_page_end(response);
        import_count++;
    }

    librdf_free_node(graph);

    return response;
}

redhttp_response_t *handle_load_post(redhttp_request_t * request, void *user_data)
{
    const char *uri_arg = redhttp_request_get_argument(request, "uri");
    const char *base_arg = redhttp_request_get_argument(request, "base-uri");
    const char *graph_arg = redhttp_request_get_argument(request, "graph");
    librdf_uri *uri = NULL, *base_uri = NULL, *graph_uri = NULL;
    redhttp_response_t *response = NULL;
    librdf_parser *parser = NULL;
    librdf_stream *stream = NULL;

    if (!uri_arg) {
        response = redstore_error_page(REDSTORE_INFO, REDHTTP_BAD_REQUEST, "Missing URI to load");
        goto CLEANUP;
    }

    uri = librdf_new_uri(world, (const unsigned char *) uri_arg);
    if (!uri) {
        response =
            redstore_error_page(REDSTORE_ERROR, REDHTTP_BAD_REQUEST,
                                "librdf_new_uri failed for URI");
        goto CLEANUP;
    }

    if (base_arg) {
        base_uri = librdf_new_uri(world, (const unsigned char *) base_arg);
    } else {
        base_uri = librdf_new_uri(world, (const unsigned char *) uri_arg);
    }
    if (!base_uri) {
        response =
            redstore_error_page(REDSTORE_ERROR, REDHTTP_BAD_REQUEST,
                                "librdf_new_uri failed for Base URI");
        goto CLEANUP;
    }

    if (graph_arg) {
        graph_uri = librdf_new_uri(world, (const unsigned char *) graph_arg);
    } else {
        graph_uri = librdf_new_uri(world, (const unsigned char *) uri_arg);
    }
    if (!graph_uri) {
        response =
            redstore_error_page(REDSTORE_ERROR, REDHTTP_BAD_REQUEST,
                                "librdf_new_uri failed for Graph URI");
        goto CLEANUP;
    }

    redstore_info("Loading URI: %s", librdf_uri_as_string(uri));
    redstore_debug("Base URI: %s", librdf_uri_as_string(base_uri));
    redstore_debug("Graph URI: %s", librdf_uri_as_string(graph_uri));

    // FIXME: allow user to choose the parser that is used
    parser = librdf_new_parser(world, "guess", NULL, NULL);
    if (!parser) {
        response =
            redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                "Failed to create parser");
        goto CLEANUP;
    }

    stream = librdf_parser_parse_as_stream(parser, uri, base_uri);
    if (!stream) {
        response =
            redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                "Failed to parse RDF as stream.");
        goto CLEANUP;
    }


    response = load_stream_into_graph(stream, graph_uri);


  CLEANUP:
    if (stream)
        librdf_free_stream(stream);
    if (parser)
        librdf_free_parser(parser);
    if (graph_uri)
        librdf_free_uri(graph_uri);
    if (base_uri)
        librdf_free_uri(base_uri);
    if (uri)
        librdf_free_uri(uri);

    return response;
}

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


redhttp_response_t *handle_graph_index(redhttp_request_t * request, void *user_data)
{
    redhttp_response_t *response = redhttp_response_new(REDHTTP_OK, NULL);
    librdf_iterator *iterator = NULL;

    page_append_html_header(response, "Named Graphs");

    iterator = librdf_storage_get_contexts(storage);
    if (!iterator) {
        return redstore_error_page(REDSTORE_ERROR,
                                   REDHTTP_INTERNAL_SERVER_ERROR, "Failed to get list of graphs.");
    }

    redhttp_response_content_append(response, "<ul>\n");
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
        redhttp_response_content_append(response,
                                        "<li><a href=\"/data/%s\">%s</a></li>\n",
                                        escaped, librdf_uri_as_string(uri));
        free(escaped);

        librdf_iterator_next(iterator);
    }
    redhttp_response_content_append(response, "</ul>\n");

    librdf_free_iterator(iterator);

    page_append_html_footer(response);

    return response;
}

static librdf_node *get_graph_context(redhttp_request_t * request)
{
    const char *uri = redhttp_request_get_path_glob(request);
    librdf_node *context;

    if (!uri || strlen(uri) < 1) {
        return NULL;
    }
    // Create node
    context = librdf_new_node_from_uri_string(world, (const unsigned char *) uri);
    if (!context) {
        redstore_debug("Failed to create node from: %s", uri);
        return NULL;
    }
    // Check if the graph exists
    if (!librdf_model_contains_context(model, context)) {
        redstore_debug("Graph not found: %s", uri);
        // FIXME: display a better error message
        return NULL;
    }

    return context;
}

redhttp_response_t *handle_graph_head(redhttp_request_t * request, void *user_data)
{
    librdf_node *context = get_graph_context(request);
    redhttp_response_t *response;

    if (context) {
        response = redhttp_response_new(REDHTTP_OK, NULL);
        librdf_free_node(context);
    } else {
        response = redhttp_response_new(REDHTTP_NOT_FOUND, "Graph not found");
    }

    return response;
}

redhttp_response_t *handle_graph_get(redhttp_request_t * request, void *user_data)
{
    librdf_node *context = get_graph_context(request);
    librdf_stream *stream;
    redhttp_response_t *response;

    if (!context) {
        return redstore_error_page(REDSTORE_INFO, REDHTTP_NOT_FOUND, "Graph not found.");
    }
    // Stream the graph
    stream = librdf_model_context_as_stream(model, context);
    if (!stream) {
        librdf_free_node(context);
        return redstore_error_page(REDSTORE_ERROR,
                                   REDHTTP_INTERNAL_SERVER_ERROR, "Failed to stream context.");
    }

    response = format_graph_stream(request, stream);

    librdf_free_stream(stream);
    librdf_free_node(context);

    return response;
}

redhttp_response_t *handle_graph_put(redhttp_request_t * request, void *user_data)
{
    const char *uri = redhttp_request_get_path_glob(request);
    const char *content_length = redhttp_request_get_header(request, "Content-Length");
    const char *content_type = redhttp_request_get_header(request, "Content-Type");
    const char *parser_name = NULL;
    redhttp_response_t *response = NULL;
    librdf_stream *stream = NULL;
    librdf_parser *parser = NULL;
    librdf_uri *graph_uri = NULL;
    unsigned char *buffer = NULL;

    if (!uri || strlen(uri) < 1) {
        return redstore_error_page(REDSTORE_INFO, REDHTTP_BAD_REQUEST, "Invalid Graph URI.");
    }
    // FIXME: stream the input data, instead of storing it in memory first

    // Check we have a content_length header
    if (!content_length) {
        response =
            redstore_error_page(REDSTORE_INFO, REDHTTP_BAD_REQUEST,
                                "Missing content length header.");
        goto CLEANUP;
    }
    // Allocate memory and read in the input data
    buffer = malloc(atoi(content_length));
    if (!buffer) {
        response =
            redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                "Failed to allocate memory for input data.");
        goto CLEANUP;
    }
    // FIXME: do this better and check for errors
    fread(buffer, 1, atoi(content_length), redhttp_request_get_socket(request));

    graph_uri = librdf_new_uri(world, (const unsigned char *) uri);
    if (!graph_uri) {
        response =
            redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                "Failed to create graph URI.");
        goto CLEANUP;
    }

    parser_name = librdf_parser_guess_name2(world, content_type, buffer, NULL);
    if (!parser_name) {
        response =
            redstore_error_page(REDSTORE_INFO, REDHTTP_INTERNAL_SERVER_ERROR,
                                "Failed to guess parser type.");
        goto CLEANUP;
    }
    redstore_debug("Parsing using: %s", parser_name);

    parser = librdf_new_parser(world, parser_name, NULL, NULL);
    if (!parser) {
        response =
            redstore_error_page(REDSTORE_INFO, REDHTTP_INTERNAL_SERVER_ERROR,
                                "Failed to create parser.");
        goto CLEANUP;
    }

    stream = librdf_parser_parse_string_as_stream(parser, buffer, graph_uri);
    if (!stream) {
        response =
            redstore_error_page(REDSTORE_INFO, REDHTTP_INTERNAL_SERVER_ERROR,
                                "Failed to parse data.");
        goto CLEANUP;
    }
    // FIXME: delete existing statements first

    response = load_stream_into_graph(stream, graph_uri);

  CLEANUP:
    if (stream)
        librdf_free_stream(stream);
    if (parser)
        librdf_free_parser(parser);
    if (graph_uri)
        librdf_free_uri(graph_uri);
    if (buffer)
        free(buffer);

    return response;
}

redhttp_response_t *handle_graph_delete(redhttp_request_t * request, void *user_data)
{
    librdf_node *context = get_graph_context(request);
    redhttp_response_t *response;

    // Create node
    if (!context) {
        return redstore_error_page(REDSTORE_INFO, REDHTTP_NOT_FOUND, "Graph not found.");
    }

    redstore_info("Deleting graph: %s", redhttp_request_get_path_glob(request));

    if (librdf_model_context_remove_statements(model, context)) {
        response =
            redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                "Error while trying to delete graph");
    } else {
        response = redstore_error_page(REDSTORE_INFO, REDHTTP_OK, "Successfully deleted Graph");
    }

    librdf_free_node(context);

    return response;
}

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


static redhttp_response_t *load_data_from_request(redhttp_request_t * request, const char* graph_uri_str, int delete_old_data)
{
    const char *content_length_str = redhttp_request_get_header(request, "Content-Length");
    const char *content_type = redhttp_request_get_header(request, "Content-Type");
    const char *base_uri_str = redhttp_request_get_argument(request, "base-uri");
    const char *parser_name = NULL;
    redhttp_response_t *response = NULL;
    librdf_stream *stream = NULL;
    librdf_parser *parser = NULL;
    librdf_uri *graph_uri = NULL;
    librdf_uri *base_uri = NULL;
    unsigned char *buffer = NULL;
    size_t content_length;
    size_t data_read;
    // FIXME: stream the input data, instead of storing it in memory first

    // Check we have a content_length header
    if (content_length_str) {
        content_length = atoi(content_length_str);
        if (content_length <= 0) {
            response =
                redstore_error_page(REDSTORE_INFO, REDHTTP_BAD_REQUEST,
                                    "Invalid content length header.");
            goto CLEANUP;
        }
    } else {
        response =
            redstore_error_page(REDSTORE_INFO, REDHTTP_BAD_REQUEST,
                                "Missing content length header.");
        goto CLEANUP;
    }

    // Allocate memory and read in the input data
    buffer = malloc(content_length);
    if (!buffer) {
        response =
            redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                "Failed to allocate memory for input data.");
        goto CLEANUP;
    }

    data_read = fread(buffer, 1, content_length, redhttp_request_get_socket(request));
    if (data_read != content_length) {
        response =
            redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                "Error reading content from client.");
        goto CLEANUP;
    }

    if (graph_uri_str) {
        graph_uri = librdf_new_uri(world, (const unsigned char *) graph_uri_str);
        if (!graph_uri) {
            response =
                redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                    "Failed to create graph URI.");
            goto CLEANUP;
        }
        redstore_debug("graph-uri: %s", graph_uri_str);
    }
    
    if (base_uri_str) {
        base_uri = librdf_new_uri(world, (unsigned char*)base_uri_str);
        if (!base_uri) {
            response =
                redstore_error_page(REDSTORE_INFO, REDHTTP_BAD_REQUEST,
                                    "Failed to create base URI.");
            goto CLEANUP;
        }
        redstore_debug("base-uri: %s", base_uri_str);
    } else if (graph_uri) {
        base_uri = librdf_new_uri_from_uri(graph_uri);
        if (!base_uri) {
            response =
                redstore_error_page(REDSTORE_INFO, REDHTTP_BAD_REQUEST,
                                    "Failed to create base URI from graph URI.");
            goto CLEANUP;
        }
    } else {
        redstore_debug("Warning: base-uri is not set");
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

    stream = librdf_parser_parse_counted_string_as_stream(parser, buffer, data_read, base_uri);
    if (!stream) {
        response =
            redstore_error_page(REDSTORE_INFO, REDHTTP_INTERNAL_SERVER_ERROR,
                                "Failed to parse data.");
        goto CLEANUP;
    }

    if (delete_old_data && graph_uri) {
        librdf_node *context = librdf_new_node_from_uri(world, graph_uri);
        if (context) {
            librdf_model_context_remove_statements(model, context);
            librdf_free_node(context);
        }
    }

    response = load_stream_into_graph(request, stream, graph_uri);

  CLEANUP:
    if (stream)
        librdf_free_stream(stream);
    if (parser)
        librdf_free_parser(parser);
    if (graph_uri)
        librdf_free_uri(graph_uri);
    if (base_uri)
        librdf_free_uri(base_uri);
    if (buffer)
        free(buffer);

    return response;
}

redhttp_response_t *handle_data_get(redhttp_request_t * request, void *user_data)
{
    librdf_stream *stream = NULL;
    redhttp_response_t *response = NULL;

    stream = librdf_model_as_stream(model);
    if (!stream) {
        return redstore_error_page(REDSTORE_ERROR,
                                   REDHTTP_INTERNAL_SERVER_ERROR, "Failed to stream model.");
    }

    response = format_graph_stream(request, stream);
    librdf_free_stream(stream);

    return response;
}

redhttp_response_t *handle_data_delete(redhttp_request_t * request, void *user_data)
{
    librdf_stream *stream = NULL;
    int err = 0;

    // FIXME: more efficient way of doing this?
    // FIXME: re-implement using librdf_model_remove_statements()
    stream = librdf_model_as_stream(model);
    if (!stream) {
        return redstore_error_page(REDSTORE_ERROR,
                                   REDHTTP_INTERNAL_SERVER_ERROR, "Failed to stream model.");
    }

    while (!librdf_stream_end(stream)) {
        librdf_statement *statement = (librdf_statement *) librdf_stream_get_object(stream);
        librdf_node *context = (librdf_node *) librdf_stream_get_context(stream);
        if (!statement) {
            redstore_error("librdf_stream_next returned NULL in handle_data_delete()");
            break;
        }

        if (librdf_model_context_remove_statement(model, context, statement))
            err++;

        librdf_stream_next(stream);
    }

    librdf_free_stream(stream);

    if (err) {
        return redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                   "Error deleting some statements.");
    } else {
        return redstore_error_page(REDSTORE_INFO, REDHTTP_OK,
                                   "Successfully deleted all statements.");
    }
}

redhttp_response_t *handle_data_post(redhttp_request_t * request, void *user_data)
{
    return load_data_from_request(request, NULL, 0);
}

static librdf_node *get_graph_context(redhttp_request_t * request)
{
    const char *uri = redhttp_request_get_path_glob(request);
    librdf_node *context = NULL;

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
        librdf_free_node(context);
        return NULL;
    }

    return context;
}

redhttp_response_t *handle_data_context_head(redhttp_request_t * request, void *user_data)
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

redhttp_response_t *handle_data_context_get(redhttp_request_t * request, void *user_data)
{
    librdf_node *context = get_graph_context(request);
    librdf_stream *stream = NULL;
    redhttp_response_t *response = NULL;

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

redhttp_response_t *handle_data_context_post(redhttp_request_t * request, void *user_data)
{
    const char *graph_uri = redhttp_request_get_path_glob(request);

    if (!graph_uri || strlen(graph_uri) < 1) {
        return redstore_error_page(REDSTORE_INFO, REDHTTP_BAD_REQUEST, "Invalid Graph URI.");
    }

    return load_data_from_request(request, graph_uri, 0);
}

redhttp_response_t *handle_data_context_put(redhttp_request_t * request, void *user_data)
{
    const char *graph_uri = redhttp_request_get_path_glob(request);

    if (!graph_uri || strlen(graph_uri) < 1) {
        return redstore_error_page(REDSTORE_INFO, REDHTTP_BAD_REQUEST, "Invalid Graph URI.");
    }

    return load_data_from_request(request, graph_uri, 1);
}

redhttp_response_t *handle_data_context_delete(redhttp_request_t * request, void *user_data)
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
        char *format_str = redstore_get_format(request, "text/plain,text/html,application/xhtml+xml");
        if (redstore_is_html_format(format_str)) {
            response = redstore_page_new("Success");
            redstore_page_append_string(response, "<p>Successfully deleted graph.</p>");
            redstore_page_end(response);
        } else {
            char *text = calloc(1, BUFSIZ);
            response = redhttp_response_new_with_type(REDHTTP_OK, NULL, "text/plain");
            snprintf(text, BUFSIZ, "Successfully deleted graph.\n");
            redhttp_response_set_content(response, text, BUFSIZ);
        }
        free(format_str);
    }

    librdf_free_node(context);

    return response;
}

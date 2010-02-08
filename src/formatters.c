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


/*
  Return the first item in the accept header

  FIXME: do content negotiation properly
*/
static char *parse_accept_header(redhttp_request_t * request)
{
    const char *accept_str = redhttp_request_get_header(request, "Accept");
    int pos = -1;
    int i;

    if (accept_str == NULL)
        return NULL;

    for (i = 0; i <= strlen(accept_str); i++) {
        if (accept_str[i] == '\0' ||
            accept_str[i] == ' ' || accept_str[i] == ',' || accept_str[i] == ';') {
            pos = i;
            break;
        }
    }

    if (pos > 0) {
        char *result = malloc(pos + 1);
        strncpy(result, accept_str, pos);
        result[pos] = '\0';
        return result;
    }

    return NULL;
}

static const char *get_format(redhttp_request_t * request)
{
    const char *format_str;

    format_str = redhttp_request_get_argument(request, "format");
    redstore_debug("format_str: %s", format_str);
    if (!format_str) {
        format_str = parse_accept_header(request);
        redstore_debug("accept: %s", format_str);
    }

    return format_str;
}


redhttp_response_t *format_graph_stream_librdf(redhttp_request_t * request,
                                               librdf_stream * stream, const char *format_str)
{
    FILE *socket = redhttp_request_get_socket(request);
    redhttp_response_t *response;
    librdf_serializer *serialiser;
    const char *format_name = NULL;
    const char *mime_type = NULL;
    int i;

    for (i = 0; serialiser_info[i].name; i++) {
        if (strcmp(format_str, serialiser_info[i].name) == 0 ||
            strcmp(format_str, serialiser_info[i].uri) == 0 ||
            strcmp(format_str, serialiser_info[i].mime_type) == 0) {
            format_name = serialiser_info[i].name;
            mime_type = serialiser_info[i].mime_type;
            break;
        }
    }

    if (!format_name) {
        return redstore_error_page(REDSTORE_INFO, REDHTTP_INTERNAL_SERVER_ERROR,
                                   "Unknown file format.");
    }

    serialiser = librdf_new_serializer(world, format_name, NULL, NULL);
    if (!serialiser) {
        return redstore_error_page(REDSTORE_ERROR,
                                   REDHTTP_INTERNAL_SERVER_ERROR, "Failed to create serialised.");
    }
    // Send back the response headers
    response = redhttp_response_new(REDHTTP_OK, NULL);
    redhttp_response_add_header(response, "Content-Type", mime_type);
    redhttp_response_send(response, request);

    if (librdf_serializer_serialize_stream_to_file_handle(serialiser, socket, NULL, stream)) {
        redstore_error("Failed to serialize graph");
        // FIXME: send error message to client?
    }

    librdf_free_serializer(serialiser);

    return response;
}


redhttp_response_t *format_graph_stream_html(redhttp_request_t * request,
                                             librdf_stream * stream, const char *format_str)
{
    redhttp_response_t *response = redhttp_response_new(REDHTTP_OK, NULL);
    FILE *socket = redhttp_request_get_socket(request);

    // Send back the response headers
    redhttp_response_add_header(response, "Content-Type", "text/html");
    redhttp_response_send(response, request);
    fprintf(socket, "<html><head><title>RedStore</title></head><body>");

    fprintf(socket, "<table class=\"triples\" border=\"1\">\n");
    fprintf(socket, "<tr><th>Subject</th><th>Predicate</th><th>Object</th></tr>\n");
    while (!librdf_stream_end(stream)) {
        librdf_statement *statement = librdf_stream_get_object(stream);
        if (!statement) {
            fprintf(stderr, "librdf_stream_next returned NULL\n");
            break;
        }

        fprintf(socket, "<tr><td>");
        librdf_node_print(librdf_statement_get_subject(statement), socket);
        fprintf(socket, "</td><td>");
        librdf_node_print(librdf_statement_get_predicate(statement), socket);
        fprintf(socket, "</td><td>");
        librdf_node_print(librdf_statement_get_object(statement), socket);
        fprintf(socket, "</td></tr>\n");

        librdf_stream_next(stream);
    }

    fprintf(socket, "</table></body></html>\n");

    return response;
}


redhttp_response_t *format_graph_stream(redhttp_request_t * request, librdf_stream * stream)
{
    redhttp_response_t *response;
    const char *format_str;

    format_str = get_format(request);
    if (format_str == NULL ||
        strcmp(format_str, "html") == 0 ||
        strcmp(format_str, "application/xml") == 0 || strcmp(format_str, "text/html") == 0) {
        response = format_graph_stream_html(request, stream, format_str);
    } else {
        response = format_graph_stream_librdf(request, stream, format_str);
    }

    return response;
}


redhttp_response_t *format_bindings_query_result_librdf(redhttp_request_t *
                                                        request,
                                                        librdf_query_results *
                                                        results, const char *format_str)
{
    FILE *socket = redhttp_request_get_socket(request);
    raptor_iostream *iostream = NULL;
    redhttp_response_t *response = NULL;
    librdf_query_results_formatter *formatter = NULL;
    const char *mime_type = NULL;
    unsigned int i;


    for (i = 0; 1; i++) {
        const char *name, *mime;
        const unsigned char *uri;
        if (librdf_query_results_formats_enumerate(world, i, &name, NULL, &uri, &mime))
            break;
        if ((name && strcmp(format_str, name) == 0) ||
            (uri && strcmp(format_str, (char *) uri) == 0) ||
            (mime && strcmp(format_str, mime) == 0)) {
            formatter = librdf_new_query_results_formatter(results, name, NULL);
            mime_type = mime;
            break;
        }
    }

    if (!formatter) {
        return redstore_error_page(REDSTORE_INFO, REDHTTP_INTERNAL_SERVER_ERROR,
                                   "Failed to match file format.");
    }

    iostream = raptor_new_iostream_to_file_handle(socket);
    if (!iostream) {
        librdf_free_query_results_formatter(formatter);
        return redstore_error_page(REDSTORE_ERROR,
                                   REDHTTP_INTERNAL_SERVER_ERROR,
                                   "Failed to create raptor_iostream for results output.");
    }
    // Send back the response headers
    response = redhttp_response_new(REDHTTP_OK, NULL);
    if (mime_type)
        redhttp_response_add_header(response, "Content-Type", mime_type);
    redhttp_response_send(response, request);

    // Stream results back to client
    if (librdf_query_results_formatter_write(iostream, formatter, results, NULL)) {
        redstore_error("Failed to serialise query results");
        // FIXME: send something to the browser?
    }

    raptor_free_iostream(iostream);
    librdf_free_query_results_formatter(formatter);

    return response;
}

redhttp_response_t *format_bindings_query_result_html(redhttp_request_t *
                                                      request,
                                                      librdf_query_results *
                                                      results, const char *format_str)
{
    redhttp_response_t *response = redhttp_response_new(REDHTTP_OK, NULL);
    FILE *socket = redhttp_request_get_socket(request);
    int i, count;

    // Send back the response headers
    redhttp_response_add_header(response, "Content-Type", "text/html");
    redhttp_response_send(response, request);
    fprintf(socket, "<html><head><title>RedStore</title></head><body>");

    count = librdf_query_results_get_bindings_count(results);
    fprintf(socket, "<table class=\"sparql\" border=\"1\">\n<tr>");
    for (i = 0; i < count; i++) {
        const char *name = librdf_query_results_get_binding_name(results, i);
        fprintf(socket, "<th>?%s</th>", name);
    }
    fprintf(socket, "</tr>\n");

    while (!librdf_query_results_finished(results)) {
        fprintf(socket, "<tr>");
        for (i = 0; i < count; i++) {
            librdf_node *value = librdf_query_results_get_binding_value(results, i);
            fprintf(socket, "<td>");
            if (value) {
                librdf_node_print(value, socket);
                librdf_free_node(value);
            } else {
                fputs("NULL", socket);
            }
            fprintf(socket, "</td>");
        }
        fprintf(socket, "</tr>\n");
        librdf_query_results_next(results);
    }

    fprintf(socket, "</table></body></html>\n");

    return response;
}

redhttp_response_t *format_bindings_query_result_text(redhttp_request_t *
                                                      request,
                                                      librdf_query_results *
                                                      results, const char *format_str)
{
    redhttp_response_t *response = redhttp_response_new(REDHTTP_OK, NULL);
    FILE *socket = redhttp_request_get_socket(request);
    int i, count;

    // Send back the response headers
    redhttp_response_add_header(response, "Content-Type", "text/plain");
    redhttp_response_send(response, request);

    count = librdf_query_results_get_bindings_count(results);
    for (i = 0; i < count; i++) {
        const char *name = librdf_query_results_get_binding_name(results, i);
        fprintf(socket, "?%s", name);
        if (i != count - 1)
            fprintf(socket, "\t");
    }
    fprintf(socket, "\n");

    while (!librdf_query_results_finished(results)) {
        for (i = 0; i < count; i++) {
            librdf_node *value = librdf_query_results_get_binding_value(results, i);
            if (value) {
                char *str = (char *) librdf_node_to_string(value);
                if (str) {
                    fputs(str, socket);
                    free(str);
                }
                librdf_free_node(value);
            } else {
                fputs("NULL", socket);
            }
            if (i != count - 1)
                fprintf(socket, "\t");
        }
        fprintf(socket, "\n");
        librdf_query_results_next(results);
    }

    return response;
}

redhttp_response_t *format_bindings_query_result(redhttp_request_t * request,
                                                 librdf_query_results * results)
{
    redhttp_response_t *response;
    const char *format_str;

    format_str = get_format(request);
    if (format_str == NULL || strcmp(format_str, "html") == 0) {
        response = format_bindings_query_result_html(request, results, format_str);
    } else if (strcmp(format_str, "text") == 0) {
        response = format_bindings_query_result_text(request, results, format_str);
    } else {
        response = format_bindings_query_result_librdf(request, results, format_str);
    }

    redstore_debug("Query returned %d results", librdf_query_results_get_count(results));

    return response;
}

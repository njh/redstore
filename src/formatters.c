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

static void node_print_xml(librdf_node * node, raptor_iostream * iostream)
{
    unsigned char *str;
    size_t str_len;
    str = librdf_node_to_counted_string(node, &str_len);
    raptor_xml_escape_string_write(str, str_len, 0, iostream);
    free(str);
}

redhttp_response_t *format_graph_stream_librdf(redhttp_request_t * request,
                                               librdf_stream * stream, const char *format_str)
{
    FILE *socket = redhttp_request_get_socket(request);
    raptor_world *raptor_world = librdf_world_get_raptor(world);
    redhttp_response_t *response;
    librdf_serializer *serialiser;
    const char *format_name = NULL;
    const char *mime_type = NULL;
    unsigned int i,j;

    for (i = 0; format_name == NULL; i++) {
        const raptor_syntax_description *desc = NULL;
        desc = raptor_world_get_serializer_description(raptor_world, i);
        if (desc == NULL)
            break;

        // Does it match a format name?
        for(j=0; desc->names[j]; j++) {
            if (strcmp(format_str, desc->names[j]) == 0) {
                format_name = desc->names[j];
                if (desc->mime_types_count)
                    mime_type = desc->mime_types[0].mime_type;
                break;
            }
        }

        // Does it match a MIME type?
        for(j=0; j < desc->mime_types_count; j++) {
            raptor_type_q mt = desc->mime_types[j];
            if (strcmp(format_str, mt.mime_type) == 0) {
                format_name = desc->names[0];
                mime_type = mt.mime_type;
                break;
            }
        }
    }

    if (!format_name) {
        return redstore_error_page(REDSTORE_INFO, REDHTTP_NOT_ACCEPTABLE,
                                   "Result format not supported for graph query type.");
    }

    serialiser = librdf_new_serializer(world, format_name, NULL, NULL);
    if (!serialiser) {
        return redstore_error_page(REDSTORE_ERROR,
                                   REDHTTP_INTERNAL_SERVER_ERROR, "Failed to create serialiser.");
    }
    // Send back the response headers
    response = redhttp_response_new(REDHTTP_OK, NULL);
    if (mime_type)
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
    raptor_iostream *iostream = NULL;
    raptor_world *raptor = librdf_world_get_raptor(world);

    // Send back the response headers
    redhttp_response_add_header(response, "Content-Type", "text/html");
    redhttp_response_send(response, request);

    iostream = raptor_new_iostream_to_file_handle(raptor, socket);
    raptor_iostream_string_write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n", iostream);
    raptor_iostream_string_write("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n", iostream);
    raptor_iostream_string_write("          \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n", iostream);
    raptor_iostream_string_write("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n", iostream);
    raptor_iostream_string_write("<head><title>RedStore</title></head><body>", iostream);
    raptor_iostream_string_write("<table class=\"triples\" border=\"1\">\n", iostream);
    raptor_iostream_string_write("<tr><th>Subject</th><th>Predicate</th><th>Object</th></tr>\n",
                                 iostream);
    while (!librdf_stream_end(stream)) {
        librdf_statement *statement = librdf_stream_get_object(stream);
        if (!statement) {
            redstore_error("librdf_stream_next returned NULL");
            break;
        }

        raptor_iostream_string_write("<tr><td>", iostream);
        node_print_xml(librdf_statement_get_subject(statement), iostream);
        raptor_iostream_string_write("</td><td>", iostream);
        node_print_xml(librdf_statement_get_predicate(statement), iostream);
        raptor_iostream_string_write("</td><td>", iostream);
        node_print_xml(librdf_statement_get_object(statement), iostream);
        raptor_iostream_string_write("</td></tr>\n", iostream);

        librdf_stream_next(stream);
    }

    raptor_iostream_string_write("</table></body></html>\n", iostream);
    raptor_free_iostream(iostream);

    return response;
}


// We need to be able to cast librdf_node to raptor_term
#ifndef LIBRDF_USE_RAPTOR_TERM
#error LIBRDF_USE_RAPTOR_TERM must be set to be able to cast librdf_node to raptor_term
#endif

redhttp_response_t *format_graph_stream_nquads(redhttp_request_t * request,
                                               librdf_stream * stream, const char *format_str)
{
    raptor_world *raptor = librdf_world_get_raptor(world);
    FILE *socket = redhttp_request_get_socket(request);
    redhttp_response_t *response = NULL;
    raptor_iostream *iostream = NULL;

    // Create an iostream
    iostream = raptor_new_iostream_to_file_handle(raptor, socket);
    if (!iostream) {
        return redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                   "Failed to create raptor_iostream when serialising nquads.");
    }

    // Send back the response headers
    response = redhttp_response_new(REDHTTP_OK, NULL);
    redhttp_response_add_header(response, "Content-Type", "text/x-nquads");
    redhttp_response_send(response, request);
    raptor_iostream_string_write("# This data is in the N-Quads format\n", iostream);
    raptor_iostream_string_write("# http://sw.deri.org/2008/07/n-quads/\n", iostream);
    raptor_iostream_string_write("#\n", iostream);

    while (!librdf_stream_end(stream)) {
        librdf_statement *statement = librdf_stream_get_object(stream);
        const raptor_term *subject, *predicate, *object, *context;

        if (!statement)
            break;

        subject = (const raptor_term*)librdf_statement_get_subject(statement);
        predicate = (const raptor_term*)librdf_statement_get_predicate(statement);
        object = (const raptor_term *)librdf_statement_get_object(statement);
        context = (const raptor_term *)librdf_stream_get_context(stream);

        raptor_term_ntriples_write(subject, iostream);
        raptor_iostream_write_byte(' ', iostream);
        raptor_term_ntriples_write(predicate, iostream);
        raptor_iostream_write_byte(' ', iostream);
        raptor_term_ntriples_write(object, iostream);
        if (context) {
            raptor_iostream_write_byte(' ', iostream);
            raptor_term_ntriples_write(context, iostream);
        }
        raptor_iostream_counted_string_write(" .\n", 3, iostream);

        librdf_stream_next(stream);
    }

    raptor_free_iostream(iostream);

    return response;
}


redhttp_response_t *format_graph_stream(redhttp_request_t * request, librdf_stream * stream)
{
    redhttp_response_t *response;
    char *format_str;

    format_str = redstore_get_format(request, accepted_serialiser_types);
    if (format_str == NULL)
        format_str = DEFAULT_GRAPH_FORMAT;

    if (redstore_is_html_format(format_str)) {
        response = format_graph_stream_html(request, stream, format_str);
    } else if (redstore_is_nquads_format(format_str)) {
        response = format_graph_stream_nquads(request, stream, format_str);
    } else {
        response = format_graph_stream_librdf(request, stream, format_str);
    }

    free(format_str);

    return response;
}


redhttp_response_t *format_bindings_query_result_librdf(redhttp_request_t *
                                                        request,
                                                        librdf_query_results *
                                                        results, const char *format_str)
{
    raptor_world *raptor = librdf_world_get_raptor(world);
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
        return redstore_error_page(REDSTORE_INFO, REDHTTP_NOT_ACCEPTABLE,
                                   "Result format not supported for bindings query type.");
    }

    iostream = raptor_new_iostream_to_file_handle(raptor, socket);
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
    raptor_world *raptor = librdf_world_get_raptor(world);
    raptor_iostream *iostream = NULL;
    FILE *socket = redhttp_request_get_socket(request);
    int i, count;

    // Send back the response headers
    redhttp_response_add_header(response, "Content-Type", "text/html");
    redhttp_response_send(response, request);

    iostream = raptor_new_iostream_to_file_handle(raptor, socket);
    raptor_iostream_string_write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n", iostream);
    raptor_iostream_string_write("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n", iostream);
    raptor_iostream_string_write("          \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n", iostream);
    raptor_iostream_string_write("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n", iostream);
    raptor_iostream_string_write("<head><title>RedStore</title></head><body>", iostream);

    count = librdf_query_results_get_bindings_count(results);
    raptor_iostream_string_write("<table class=\"sparql\" border=\"1\">\n<tr>", iostream);
    for (i = 0; i < count; i++) {
        const char *name = librdf_query_results_get_binding_name(results, i);
        // FIXME: this should be escaped
        raptor_iostream_string_write("<th>", iostream);
        raptor_iostream_string_write(name, iostream);
        raptor_iostream_string_write("</th>", iostream);
    }
    raptor_iostream_string_write("</tr>\n", iostream);

    while (!librdf_query_results_finished(results)) {
        raptor_iostream_string_write("<tr>", iostream);
        for (i = 0; i < count; i++) {
            librdf_node *value = librdf_query_results_get_binding_value(results, i);
            raptor_iostream_string_write("<td>", iostream);
            if (value) {
                node_print_xml(value, iostream);
                librdf_free_node(value);
            } else {
                raptor_iostream_string_write("NULL", iostream);
            }
            raptor_iostream_string_write("</td>", iostream);
        }
        raptor_iostream_string_write("</tr>\n", iostream);
        librdf_query_results_next(results);
    }

    raptor_iostream_string_write("</table></body></html>\n", iostream);

    raptor_free_iostream(iostream);

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
    char *format_str;

    format_str = redstore_get_format(request, accepted_query_result_types);
    if (format_str == NULL)
        format_str = DEFAULT_RESULTS_FORMAT;

    if (redstore_is_html_format(format_str)) {
        response = format_bindings_query_result_html(request, results, format_str);
    } else if (redstore_is_text_format(format_str)) {
        response = format_bindings_query_result_text(request, results, format_str);
    } else {
        response = format_bindings_query_result_librdf(request, results, format_str);
    }
    free(format_str);

    redstore_debug("Query returned %d results", librdf_query_results_get_count(results));

    return response;
}

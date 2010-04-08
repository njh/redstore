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


// This method is a copy of librdf_node_write()
// which is not guaranteed to continue being in the ntriples format
static int node_write_ntriple(librdf_node * node, raptor_iostream * iostr)
{
    static const unsigned char const null_string[] = "(null)";
    const unsigned char *str = NULL;
    size_t len;

    if (!node) {
        raptor_iostream_counted_string_write(null_string, sizeof(null_string), iostr);
        return 0;
    }

    switch (librdf_node_get_type(node)) {
    case LIBRDF_NODE_TYPE_LITERAL:
        raptor_iostream_write_byte('"', iostr);
        str = librdf_node_get_literal_value_as_counted_string(node, &len);
        raptor_string_ntriples_write(str, len, '"', iostr);
        raptor_iostream_write_byte('"', iostr);
        if (librdf_node_get_literal_value_language(node)) {
            raptor_iostream_write_byte('@', iostr);
            raptor_iostream_string_write(librdf_node_get_literal_value_language(node), iostr);
        }
        if (librdf_node_get_literal_value_datatype_uri(node)) {
            raptor_iostream_counted_string_write("^^<", 3, iostr);
            str = librdf_uri_as_counted_string(librdf_node_get_literal_value_datatype_uri(node),
                                               &len);
            raptor_string_ntriples_write(str, len, '>', iostr);
            raptor_iostream_write_byte('>', iostr);
        }
        break;

    case LIBRDF_NODE_TYPE_BLANK:
        raptor_iostream_counted_string_write("_:", 2, iostr);
        str = librdf_node_get_blank_identifier(node);
        len = strlen((char *) str);
        raptor_iostream_counted_string_write(str, len, iostr);
        break;

    case LIBRDF_NODE_TYPE_RESOURCE:
        raptor_iostream_write_byte('<', iostr);
        str = librdf_uri_as_counted_string(librdf_node_get_uri(node), &len);
        raptor_string_ntriples_write(str, len, '>', iostr);
        raptor_iostream_write_byte('>', iostr);
        break;

    case LIBRDF_NODE_TYPE_UNKNOWN:
    default:
        redstore_error("Unknown node type during dump");
        return 1;
    }

    return 0;
}


redhttp_response_t *handle_dump_get(redhttp_request_t * request, void *user_data)
{
    librdf_stream *stream = librdf_model_as_stream(model);
    FILE *socket = redhttp_request_get_socket(request);
    redhttp_response_t *response = NULL;
    raptor_iostream *iostream = NULL;
#ifdef RAPTOR_V2_AVAILABLE
    raptor_world *raptor = librdf_world_get_raptor(world);
#endif

    // Create an iostream
#ifdef RAPTOR_V2_AVAILABLE
    iostream = raptor_new_iostream_to_file_handle(raptor, socket);
#else
    iostream = raptor_new_iostream_to_file_handle(socket);
#endif
    if (!iostream)
        return NULL;

    // Send back the response headers
    response = redhttp_response_new(REDHTTP_OK, NULL);
    redhttp_response_add_header(response, "Content-Type", "text/x-nquads");
    redhttp_response_send(response, request);
    raptor_iostream_string_write("# This is an N-Quads dump from RedStore\n", iostream);
    raptor_iostream_string_write("# http://sw.deri.org/2008/07/n-quads/\n", iostream);
    raptor_iostream_string_write("#\n", iostream);

    while (!librdf_stream_end(stream)) {
        librdf_statement *statement = librdf_stream_get_object(stream);
        librdf_node *context = (librdf_node *) librdf_stream_get_context(stream);
        if (!statement)
            break;

        node_write_ntriple(librdf_statement_get_subject(statement), iostream);
        raptor_iostream_write_byte(' ', iostream);
        node_write_ntriple(librdf_statement_get_predicate(statement), iostream);
        raptor_iostream_write_byte(' ', iostream);
        node_write_ntriple(librdf_statement_get_object(statement), iostream);
        raptor_iostream_write_byte(' ', iostream);
        node_write_ntriple(context, iostream);
        raptor_iostream_counted_string_write(" .\n", 3, iostream);

        librdf_stream_next(stream);
    }

    raptor_free_iostream(iostream);

    return response;
}

/*
    RedStore - a lightweight RDF triplestore powered by Redland
    Copyright (C) 2010-2011 Nicholas J Humfrey <njh@aelius.com>

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


redhttp_response_t *format_graph_stream(redhttp_request_t * request, librdf_stream * stream)
{
  FILE *socket = redhttp_request_get_socket(request);
  const raptor_syntax_description* desc = NULL;
  redhttp_response_t *response = NULL;
  librdf_serializer *serialiser = NULL;
  const char* mime_type = NULL;

  desc = redstore_negotiate_format(request, librdf_serializer_get_description, DEFAULT_GRAPH_FORMAT, &mime_type);
  if (!desc) {
    response = redstore_page_new_with_message(
      request, LIBRDF_LOG_INFO, REDHTTP_NOT_ACCEPTABLE,
      "Results format not supported for graph query type."
    );
    goto CLEANUP;
  }

  serialiser = librdf_new_serializer(world, desc->names[0], NULL, NULL);
  if (!serialiser) {
    response = redstore_page_new_with_message(
      request, LIBRDF_LOG_ERROR, REDHTTP_INTERNAL_SERVER_ERROR, "Failed to create serialiser."
    );
    goto CLEANUP;
  }

  // Add the namespaces used by the service description
  librdf_serializer_set_namespace(serialiser, librdf_get_concept_schema_namespace(world), "rdfs");
  librdf_serializer_set_namespace(serialiser, sd_ns_uri, "sd");
  librdf_serializer_set_namespace(serialiser, format_ns_uri, "format");
  librdf_serializer_set_namespace(serialiser, void_ns_uri, "void");

  // Send back the response headers
  response = redhttp_response_new(REDHTTP_OK, NULL);
  if (mime_type)
    redhttp_response_add_header(response, "Content-Type", mime_type);
  redhttp_response_send(response, request);

  if (librdf_serializer_serialize_stream_to_file_handle(serialiser, socket, NULL, stream)) {
    redstore_error("Failed to serialize graph");
    // FIXME: send error message to client?
  }

CLEANUP:
  if (serialiser)
    librdf_free_serializer(serialiser);

  return response;
}


redhttp_response_t *format_bindings_query_result(redhttp_request_t * request,
                                                 librdf_query_results * results)
{
  raptor_world *raptor = librdf_world_get_raptor(world);
  FILE *socket = redhttp_request_get_socket(request);
  raptor_iostream *iostream = NULL;
  redhttp_response_t *response = NULL;
  librdf_query_results_formatter *formatter = NULL;
  const raptor_syntax_description* desc = NULL;
  const char* mime_type = NULL;

  desc = redstore_negotiate_format(request, librdf_query_results_formats_get_description, DEFAULT_RESULTS_FORMAT, &mime_type);
  if (!desc) {
    response = redstore_page_new_with_message(
      request, LIBRDF_LOG_INFO, REDHTTP_NOT_ACCEPTABLE,
      "Results format not supported for bindings query type."
    );
    goto CLEANUP;
  }

  formatter = librdf_new_query_results_formatter2(results, desc->names[0], NULL, NULL);
  if (!formatter) {
    response = redstore_page_new_with_message(
      request, LIBRDF_LOG_ERROR, REDHTTP_INTERNAL_SERVER_ERROR, "Failed to create results formatter."
    );
    goto CLEANUP;
  }

  iostream = raptor_new_iostream_to_file_handle(raptor, socket);
  if (!iostream) {
    response = redstore_page_new_with_message(
      request, LIBRDF_LOG_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
      "Failed to create raptor_iostream for results output."
    );
    goto CLEANUP;
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

  redstore_debug("Query returned %d results", librdf_query_results_get_count(results));

CLEANUP:
  if (iostream)
    raptor_free_iostream(iostream);
  if (formatter)
    librdf_free_query_results_formatter(formatter);

  return response;
}

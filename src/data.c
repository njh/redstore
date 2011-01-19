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
  librdf_iterator *iterator = NULL;
  librdf_stream *stream = NULL;
  int err = 0;

  // FIXME: re-implement using librdf_model_remove_statements()

  // First:  delete all the named graphs
  iterator = librdf_storage_get_contexts(storage);
  if (!iterator) {
    return redstore_error_page(REDSTORE_ERROR,
                               REDHTTP_INTERNAL_SERVER_ERROR, "Failed to get list of graphs.");
  }

  while (!librdf_iterator_end(iterator)) {
    librdf_node *graph = (librdf_node *) librdf_iterator_get_object(iterator);
    if (!graph) {
      redstore_error("librdf_iterator_get_next returned NULL");
      break;
    }

    if (librdf_model_context_remove_statements(model, graph))
      err++;

    librdf_iterator_next(iterator);
  }
  librdf_free_iterator(iterator);


  // Second: delete the remaining triples
  stream = librdf_model_as_stream(model);
  if (!stream) {
    return redstore_error_page(REDSTORE_ERROR,
                               REDHTTP_INTERNAL_SERVER_ERROR, "Failed to stream model.");
  }

  while (!librdf_stream_end(stream)) {
    librdf_statement *statement = (librdf_statement *) librdf_stream_get_object(stream);
    if (!statement) {
      redstore_error("librdf_stream_next returned NULL in handle_data_delete()");
      break;
    }

    if (librdf_model_remove_statement(model, statement))
      err++;

    librdf_stream_next(stream);
  }
  librdf_free_stream(stream);

  if (err) {
    return redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                               "Error deleting some statements.");
  } else {
    return redstore_error_page(REDSTORE_INFO, REDHTTP_OK, "Successfully deleted all statements.");
  }
}

redhttp_response_t *handle_data_post(redhttp_request_t * request, void *user_data)
{
  return parse_data_from_request_body(request, NULL, load_stream_into_graph);
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

  return parse_data_from_request_body(request, graph_uri, load_stream_into_graph);
}

redhttp_response_t *handle_data_context_put(redhttp_request_t * request, void *user_data)
{
  const char *graph_uri = redhttp_request_get_path_glob(request);

  if (!graph_uri || strlen(graph_uri) < 1) {
    return redstore_error_page(REDSTORE_INFO, REDHTTP_BAD_REQUEST, "Invalid Graph URI.");
  }

  return parse_data_from_request_body(request, graph_uri, clear_and_load_stream_into_graph);
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
    redhttp_negotiate_t *accept =
        redhttp_negotiate_parse("text/plain,text/html,application/xhtml+xml");
    char *format_str = redstore_get_format(request, accept, "text/plain");
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
    redhttp_negotiate_free(&accept);
  }

  librdf_free_node(context);

  return response;
}

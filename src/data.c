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

static librdf_node *get_graph_node(redhttp_request_t * request)
{
  unsigned char *graph = NULL;
  librdf_uri *base_uri = NULL;
  librdf_uri *graph_uri = NULL;
  librdf_node *graph_node = NULL;

  base_uri = librdf_new_uri(world, (unsigned char*)redhttp_request_get_full_url(request));
  if (!base_uri) {
    redstore_error("Failed to create librdf_uri for current request.");
    goto CLEANUP;
  }

  graph = (unsigned char *)redhttp_request_get_argument(request, "graph");
  if (graph) {
    graph_uri = librdf_new_uri_relative_to_base(base_uri, graph);
  } else {
    graph_uri = librdf_new_uri_from_uri(base_uri);
  }

  if (graph_uri) {
    redstore_debug("Graph URI: %s", librdf_uri_as_string(graph_uri));
  } else {
    redstore_error("Failed to create librdf_uri for graph.");
    goto CLEANUP;
  }

  // Create node
  graph_node = librdf_new_node_from_uri(world, graph_uri);
  if (!graph_node) {
    redstore_error("Failed to create graph node from librdf_uri.");
  }

CLEANUP:
  if (base_uri)
    librdf_free_uri(base_uri);
  if (graph_uri)
    librdf_free_uri(graph_uri);

  return graph_node;
}

redhttp_response_t *handle_data_context_head(redhttp_request_t * request, void *user_data)
{
  librdf_node *graph_node = get_graph_node(request);
  redhttp_response_t *response = NULL;

  if (!graph_node) {
    return redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                               "Failed to get node for graph.");
  }

  if (librdf_model_contains_context(model, graph_node)) {
    response = redhttp_response_new(REDHTTP_OK, NULL);
  } else {
    response = redstore_error_page(REDSTORE_INFO, REDHTTP_NOT_FOUND, "Graph not found.");
  }

  librdf_free_node(graph_node);

  return response;
}

redhttp_response_t *handle_data_context_get(redhttp_request_t * request, void *user_data)
{
  librdf_node *graph_node = get_graph_node(request);
  librdf_stream *stream = NULL;
  redhttp_response_t *response = NULL;

  if (!graph_node) {
    return redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                               "Failed to get node for graph.");
  }

  // Check if the graph exists
  if (!librdf_model_contains_context(model, graph_node)) {
    librdf_free_node(graph_node);
    return redstore_error_page(REDSTORE_INFO, REDHTTP_NOT_FOUND, "Graph not found.");;
  }

  // Stream the graph
  stream = librdf_model_context_as_stream(model, graph_node);
  if (!stream) {
    librdf_free_node(graph_node);
    return redstore_error_page(REDSTORE_ERROR,
                               REDHTTP_INTERNAL_SERVER_ERROR, "Failed to stream context.");
  }

  response = format_graph_stream(request, stream);

  librdf_free_stream(stream);
  librdf_free_node(graph_node);

  return response;
}

redhttp_response_t *handle_data_context_post(redhttp_request_t * request, void *user_data)
{
  librdf_node *graph_node = get_graph_node(request);
  if (!graph_node) {
    return redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                               "Failed to get node for graph.");
  }

  return parse_data_from_request_body(request, graph_node, load_stream_into_graph);
}

redhttp_response_t *handle_data_context_put(redhttp_request_t * request, void *user_data)
{
  librdf_node *graph_node = get_graph_node(request);
  if (!graph_node) {
    return redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                               "Failed to get node for graph.");
  }

  return parse_data_from_request_body(request, graph_node, clear_and_load_stream_into_graph);
}

redhttp_response_t *handle_data_context_delete(redhttp_request_t * request, void *user_data)
{
  librdf_node *graph_node = get_graph_node(request);
  redhttp_response_t *response = NULL;

  if (!graph_node) {
    return redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                               "Failed to get node for graph.");
  }

  // Check if the graph exists
  if (!librdf_model_contains_context(model, graph_node)) {
    librdf_free_node(graph_node);
    return redstore_error_page(REDSTORE_INFO, REDHTTP_NOT_FOUND, "Graph not found.");;
  }

  if (librdf_model_context_remove_statements(model, graph_node)) {
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

  librdf_free_node(graph_node);

  return response;
}

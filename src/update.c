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


redhttp_response_t *load_stream_into_graph(redhttp_request_t * request, librdf_stream * stream,
                                           librdf_node * graph)
{
  redhttp_response_t *response = NULL;

  if (librdf_model_context_add_statements(model, graph, stream)) {
    return redstore_error_page(REDSTORE_ERROR,
                               REDHTTP_INTERNAL_SERVER_ERROR, "Failed to add statements to graph.");
  }
  // FIXME: check for parse errors or parse warnings
  if (!response) {
    redhttp_negotiate_t *accept =
        redhttp_negotiate_parse("text/plain,text/html,application/xhtml+xml");
    char *format_str = redstore_get_format(request, accept, "text/plain");
    const char *graph_str = NULL;

    if (graph) {
      librdf_uri *graph_uri = librdf_node_get_uri(graph);
      if (graph_uri)
        graph_str = (char *) librdf_uri_as_string(graph_uri);
    } else {
      graph_str = "the default graph";
    }

    redstore_info("Successfully added triples to graph.");
    import_count++;

    if (redstore_is_html_format(format_str)) {
      response = redstore_page_new("OK");
      redstore_page_append_string(response, "<p>Successfully added triples to ");
      redstore_page_append_escaped(response, graph_str, 0);
      redstore_page_append_string(response, "</p>\n");
      redstore_page_end(response);
    } else {
      char *text = calloc(1, BUFSIZ);
      response = redhttp_response_new_with_type(REDHTTP_OK, NULL, "text/plain");
      snprintf(text, BUFSIZ, "Successfully added triples to %s\n", graph_str);
      redhttp_response_set_content(response, text, strlen(text));
    }
    free(format_str);
    redhttp_negotiate_free(&accept);
  }

  return response;
}

redhttp_response_t *clear_and_load_stream_into_graph(redhttp_request_t * request,
                                                     librdf_stream * stream, librdf_node * graph)
{
  if (graph) {
    librdf_model_context_remove_statements(model, graph);
  }

  return load_stream_into_graph(request, stream, graph);
}

redhttp_response_t *delete_stream_from_graph(redhttp_request_t * request, librdf_stream * stream,
                                             librdf_node * graph)
{
  redhttp_negotiate_t *accept =
      redhttp_negotiate_parse("text/plain,text/html,application/xhtml+xml");
  char *format_str = redstore_get_format(request, accept, "text/plain");
  redhttp_response_t *response = NULL;
  const char *message = NULL;
  int count = 0;

  while (!librdf_stream_end(stream)) {
    librdf_statement *statement = librdf_stream_get_object(stream);
    // FIXME: this is required due to a bug in librdf v1.0.10
    if (graph) {
      if (librdf_model_context_remove_statement(model, graph, statement) == 0)
        count++;
    } else {
      if (librdf_model_remove_statement(model, statement) == 0)
        count++;
    }
    librdf_stream_next(stream);
  }

  redstore_info("Deleted %d triples.", count);
  if (count > 0) {
    message = "Successfully deleted triples.";
  } else {
    message = "No triples deleted.";
  }

  // Send the response
  if (redstore_is_html_format(format_str)) {
    response = redstore_page_new("OK");
    redstore_page_append_strings(response, "<p>", message, "</p>\n", NULL);
    redstore_page_end(response);
  } else {
    char *text = calloc(1, BUFSIZ);
    response = redhttp_response_new_with_type(REDHTTP_OK, NULL, "text/plain");
    strncpy(text, message, BUFSIZ);
    redhttp_response_set_content(response, text, strlen(text));
  }
  free(format_str);
  redhttp_negotiate_free(&accept);

  return response;
}

redhttp_response_t *parse_data_from_buffer(redhttp_request_t * request, unsigned char *buffer,
                                           size_t content_length, const char *parser_name,
                                           librdf_node *graph_node,
                                           redstore_stream_processor stream_proc)
{
  const char *base_uri_str = redhttp_request_get_argument(request, "base-uri");
  redhttp_response_t *response = NULL;
  librdf_stream *stream = NULL;
  librdf_parser *parser = NULL;
  librdf_uri *base_uri = NULL;

  if (base_uri_str) {
    base_uri = librdf_new_uri(world, (unsigned char *) base_uri_str);
    if (!base_uri) {
      response =
          redstore_error_page(REDSTORE_INFO, REDHTTP_INTERNAL_SERVER_ERROR,
                              "Failed to create base URI.");
      goto CLEANUP;
    }
    redstore_debug("base-uri: %s", base_uri_str);
  } else if (graph_node) {
    librdf_uri *graph_uri = librdf_node_get_uri(graph_node);
    base_uri = librdf_new_uri_from_uri(graph_uri);
    if (!base_uri) {
      response =
          redstore_error_page(REDSTORE_INFO, REDHTTP_INTERNAL_SERVER_ERROR,
                              "Failed to create base URI from graph URI.");
      goto CLEANUP;
    }
  } else {
    redstore_debug("Warning: neither graph nor base-uri are set");
  }

  redstore_debug("Parsing using: %s", parser_name);
  parser = librdf_new_parser(world, parser_name, NULL, NULL);
  if (!parser) {
    response =
        redstore_error_page(REDSTORE_INFO, REDHTTP_INTERNAL_SERVER_ERROR,
                            "Failed to create parser.");
    goto CLEANUP;
  }

  stream = librdf_parser_parse_counted_string_as_stream(parser, buffer, content_length, base_uri);
  if (!stream) {
    response =
        redstore_error_page(REDSTORE_INFO, REDHTTP_INTERNAL_SERVER_ERROR, "Failed to parse data.");
    goto CLEANUP;
  }

  response = stream_proc(request, stream, graph_node);

CLEANUP:
  if (stream)
    librdf_free_stream(stream);
  if (parser)
    librdf_free_parser(parser);
  if (base_uri)
    librdf_free_uri(base_uri);

  return response;
}

redhttp_response_t *parse_data_from_request_body(redhttp_request_t * request,
                                                 librdf_node *graph_node,
                                                 redstore_stream_processor stream_proc)
{
  const char *content_length_str = redhttp_request_get_header(request, "Content-Length");
  const char *content_type = redhttp_request_get_header(request, "Content-Type");
  redhttp_response_t *response = NULL;
  unsigned char *buffer = NULL;
  const char *parser_name = NULL;
  size_t content_length;
  size_t data_read;

  // Check we have a content_length header
  if (content_length_str) {
    content_length = atoi(content_length_str);
    if (content_length <= 0) {
      response =
          redstore_error_page(REDSTORE_DEBUG, REDHTTP_BAD_REQUEST,
                              "Invalid content length header.");
      goto CLEANUP;
    }
  } else {
    response =
        redstore_error_page(REDSTORE_DEBUG, REDHTTP_BAD_REQUEST, "Missing content length header.");
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

  parser_name = librdf_parser_guess_name2(world, content_type, buffer, NULL);
  if (!parser_name) {
    response =
        redstore_error_page(REDSTORE_INFO, REDHTTP_INTERNAL_SERVER_ERROR,
                            "Failed to guess parser type.");
    goto CLEANUP;
  }

  response =
      parse_data_from_buffer(request, buffer, data_read, parser_name, graph_node, stream_proc);

CLEANUP:
  if (buffer)
    free(buffer);

  return response;
}


redhttp_response_t *handle_load_post(redhttp_request_t * request, void *user_data)
{
  const char *uri_arg = redhttp_request_get_argument(request, "uri");
  const char *base_arg = redhttp_request_get_argument(request, "base-uri");
  const char *graph_arg = redhttp_request_get_argument(request, "graph");
  const char *parser_arg = redhttp_request_get_argument(request, "parser");
  librdf_uri *uri = NULL, *base_uri = NULL, *graph_uri = NULL;
  redhttp_response_t *response = NULL;
  librdf_parser *parser = NULL;
  librdf_stream *stream = NULL;
  librdf_node *graph = NULL;

  if (!uri_arg) {
    response = redstore_error_page(REDSTORE_DEBUG, REDHTTP_BAD_REQUEST, "Missing URI to load");
    goto CLEANUP;
  }

  uri = librdf_new_uri(world, (const unsigned char *) uri_arg);
  if (!uri) {
    response =
        redstore_error_page(REDSTORE_ERROR, REDHTTP_BAD_REQUEST, "librdf_new_uri failed for URI");
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

  if (parser_arg) {
    redstore_info("Parsing using: %s", parser_arg);
  } else {
    parser_arg = "guess";
  }

  parser = librdf_new_parser(world, parser_arg, NULL, NULL);
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

  graph = librdf_new_node_from_uri(world, graph_uri);
  if (!graph) {
    return redstore_error_page(REDSTORE_ERROR,
                               REDHTTP_INTERNAL_SERVER_ERROR,
                               "librdf_new_node_from_uri failed for graph-uri.");
  }

  response = load_stream_into_graph(request, stream, graph);

CLEANUP:
  if (stream)
    librdf_free_stream(stream);
  if (parser)
    librdf_free_parser(parser);
  if (graph)
    librdf_free_node(graph);
  if (graph_uri)
    librdf_free_uri(graph_uri);
  if (base_uri)
    librdf_free_uri(base_uri);
  if (uri)
    librdf_free_uri(uri);

  return response;
}


redhttp_response_t *handle_insert_post(redhttp_request_t * request, void *user_data)
{
  const char *content = redhttp_request_get_argument(request, "content");
  const char *content_type = redhttp_request_get_argument(request, "content-type");
  const char *graph_uri_str = redhttp_request_get_argument(request, "graph");
  librdf_node *graph_node = NULL;

  if (!content) {
    return redstore_error_page(REDSTORE_DEBUG, REDHTTP_BAD_REQUEST,
                               "Missing the 'content' argument.");
  }

  if (!content_type)
    content_type = DEFAULT_PARSE_FORMAT;
    
  if (graph_uri_str) {
    graph_node = librdf_new_node_from_uri_string(world, (unsigned char*)graph_uri_str);
    if (!graph_node) {
      return redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                 "Error creating graph node from URI string");
    }
  }

  return parse_data_from_buffer(request, (unsigned char *) content, strlen(content), content_type,
                                graph_node, load_stream_into_graph);
}

redhttp_response_t *handle_delete_post(redhttp_request_t * request, void *user_data)
{
  const char *content = redhttp_request_get_argument(request, "content");
  const char *content_type = redhttp_request_get_argument(request, "content-type");
  const char *graph_uri_str = redhttp_request_get_argument(request, "graph");
  librdf_node *graph_node = NULL;

  if (!content) {
    return redstore_error_page(REDSTORE_DEBUG, REDHTTP_BAD_REQUEST,
                               "Missing the 'content' argument.");
  }

  if (!content_type)
    content_type = DEFAULT_PARSE_FORMAT;
    
  if (graph_uri_str) {
    graph_node = librdf_new_node_from_uri_string(world, (unsigned char*)graph_uri_str);
    if (!graph_node) {
      return redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                 "Error creating graph node from URI string");
    }
  }

  return parse_data_from_buffer(request, (unsigned char *) content, strlen(content), content_type,
                                graph_node, delete_stream_from_graph);
}

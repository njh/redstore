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
#include <assert.h>

#include "redstore.h"


// ------- Globals -------
librdf_uri *format_ns_uri = NULL;
librdf_uri *sd_ns_uri = NULL;


/*
static librdf_node *new_node_from_integer(librdf_world * world, int i)
{
  librdf_uri *xsd_integer_uri = NULL;
  librdf_node *node = NULL;
  unsigned char *string = NULL;

#define INTEGER_BUFFER_SIZE 20
  string = (unsigned char *) malloc(INTEGER_BUFFER_SIZE + 1);
  if (!string)
    return NULL;

  // snprintf() takes as length the buffer size including NULL
  snprintf((char *) string, INTEGER_BUFFER_SIZE + 1, "%d", i);

  xsd_integer_uri =
      librdf_new_uri(world, (unsigned char *) "http://www.w3.org/2001/XMLSchema#integer");

  node = librdf_new_node_from_typed_literal(world, string, NULL, xsd_integer_uri);

  if (xsd_integer_uri)
    librdf_free_uri(xsd_integer_uri);

  return node;
}
*/

static librdf_node* sd_get_endpoint_node(const char * request_url_str)
{
  librdf_uri *request_uri = NULL, *endpoint_uri = NULL;
  librdf_node *endpoint_node = NULL;

  request_uri = librdf_new_uri(world, (unsigned char*)request_url_str);
  if (!request_uri) {
    redstore_error("Failed to create request_uri");
    goto CLEANUP;
  }

  endpoint_uri = librdf_new_uri_relative_to_base(request_uri, (unsigned char*)"sparql");
  if (!endpoint_uri) {
    redstore_error("Failed to create endpoint_uri");
    goto CLEANUP;
  }

  endpoint_node = librdf_new_node_from_uri(world, endpoint_uri);
  if (!endpoint_uri) {
    redstore_error("Failed to create endpoint_node");
    goto CLEANUP;
  }

CLEANUP:
  if (request_uri)
    librdf_free_uri(request_uri);
  if (endpoint_uri)
    librdf_free_uri(endpoint_uri);

  return endpoint_node;
}


static int context_count(librdf_storage * storage)
{
  librdf_iterator *iterator = NULL;
  int count = 0;

  assert(storage != NULL);

  iterator = librdf_storage_get_contexts(storage);
  if (!iterator) {
    redstore_error("librdf_storage_get_contexts returned NULL");
    return -1;
  }

  while (!librdf_iterator_end(iterator)) {
    count++;
    librdf_iterator_next(iterator);
  }

  librdf_free_iterator(iterator);

  return count;
}


static int sd_add_format_descriptions(librdf_model *sd_model, librdf_node *service_node, description_proc_t desc_proc, const char *type)
{
  librdf_node *format_node = NULL;
  unsigned int i,n;

  for(i=0; 1; i++) {
    const raptor_syntax_description* desc = NULL;
    desc = desc_proc(world, i);
    if (!desc)
      break;

    // Hack to remove the 'guess' format from the service description
    if (strcmp(desc->names[0], "guess") == 0)
      continue;

    // If the format has no URI, create a bnode
    if (desc->uri_strings && desc->uri_strings[0]) {
      format_node = librdf_new_node_from_uri_string(world, (const unsigned char *) desc->uri_strings[0]);
    } else {
      format_node = librdf_new_node(world);
    }

    if (!format_node) {
      redstore_error("Failed to create new node for format.");
      return -1;
    }

    librdf_model_add(sd_model,
                     librdf_new_node_from_node(format_node),
                     librdf_new_node_from_node(LIBRDF_MS_type(world)),
                     librdf_new_node_from_uri_local_name(world, format_ns_uri, (const unsigned char *) "Format")
        );

    librdf_model_add(sd_model,
                     librdf_new_node_from_node(service_node),
                     librdf_new_node_from_uri_local_name(world, sd_ns_uri,
                                                         (const unsigned char *) type),
                     librdf_new_node_from_node(format_node)
        );

    for (n = 0; desc->names[n]; n++) {
      librdf_model_add(sd_model,
                       librdf_new_node_from_node(format_node),
                       librdf_new_node_from_node(LIBRDF_S_label(world)),
                       librdf_new_node_from_literal(world, (const unsigned char *) desc->names[n],
                                                    NULL, 0)
          );
    }

    if (desc->label) {
      librdf_model_add(sd_model,
                       librdf_new_node_from_node(format_node),
                       librdf_new_node_from_node(LIBRDF_S_comment(world)),
                       librdf_new_node_from_literal(world, (const unsigned char *) desc->label, NULL,
                                                    0)
          );
    }

    for (n = 0; n < desc->mime_types_count; n++) {
      const raptor_type_q mime_type = desc->mime_types[n];
      librdf_model_add(sd_model,
                       librdf_new_node_from_node(format_node),
                       librdf_new_node_from_uri_local_name(world, format_ns_uri,
                                                           (const unsigned char *) "media_type"),
                       librdf_new_node_from_literal(world, (unsigned char *) mime_type.mime_type,
                                                    NULL, 0)
          );
    }

    if (desc->uri_strings && desc->uri_strings[0]) {
      for (n = 1; desc->uri_strings[n]; n++) {
        const char *uri_string = desc->uri_strings[n];
        librdf_model_add(sd_model,
                         librdf_new_node_from_node(format_node),
                         librdf_new_node_from_node(LIBRDF_S_seeAlso(world)),
                         librdf_new_node_from_uri_string(world,
                                                         (const unsigned char *) uri_string)
            );
      }
    }

    librdf_free_node(format_node);
  }

  return 0;
}

static int sd_add_query_languages(librdf_model *sd_model, librdf_node *service_node)
{
  int i,n;

  for(i=0; 1; i++) {
    const raptor_syntax_description* desc = librdf_query_language_get_description(world, i);
    librdf_node *lang_node = NULL;
    if (!desc)
      break;

    for (n = 0; desc->names[n]; n++) {
      if (strcmp(desc->names[n], "sparql10")==0) {
        lang_node = librdf_new_node_from_uri_local_name(world, sd_ns_uri, (unsigned char *) "SPARQL10Query");
        break;
      } else if (strcmp(desc->names[n], "sparql11-query")==0) {
        lang_node = librdf_new_node_from_uri_local_name(world, sd_ns_uri, (unsigned char *) "SPARQL11Query");
        break;
      }
    }

    if (lang_node) {
      librdf_model_add(sd_model,
                       librdf_new_node_from_node(service_node),
                       librdf_new_node_from_uri_local_name(world, sd_ns_uri,
                                                           (const unsigned char *) "supportedLanguage"),
                       librdf_new_node_from_node(lang_node)
          );

      librdf_model_add(sd_model,
                       librdf_new_node_from_node(lang_node),
                       librdf_new_node_from_node(LIBRDF_S_comment(world)),
                       librdf_new_node_from_literal(world, (const unsigned char *) desc->label, NULL, 0)
          );

      if (desc->uri_strings) {
        for (n = 0; desc->uri_strings[n]; n++) {
          librdf_model_add(sd_model,
                           librdf_new_node_from_node(lang_node),
                           librdf_new_node_from_node(LIBRDF_S_seeAlso(world)),
                           librdf_new_node_from_uri_string(world, (const unsigned char *) desc->uri_strings[n])
              );
        }
      }

      librdf_free_node(lang_node);
    }
  }

  return 0;
}

static librdf_model * create_service_description(librdf_storage *sd_storage, const char * request_url)
{
  librdf_model *sd_model = NULL;
  librdf_node *service_node = NULL;
  char *comment = NULL;

  sd_model = librdf_new_model(world, sd_storage, NULL);
  if (!sd_model) {
    redstore_error("Failed to create model for service description.");
    return NULL;
  }

  service_node = librdf_new_node(world);
  if (!service_node) {
    redstore_error("Failed to create service description bnode - librdf_new_node returned NULL");
    librdf_free_model(sd_model);
    return NULL;
  }

  librdf_model_add(sd_model,
                   librdf_new_node_from_node(service_node),
                   librdf_new_node_from_node(LIBRDF_MS_type(world)),
                   librdf_new_node_from_uri_local_name(world, sd_ns_uri, (unsigned char *) "Service")
      );

  sd_add_format_descriptions(sd_model, service_node, librdf_parser_get_description, "inputFormat");
  sd_add_format_descriptions(sd_model, service_node, librdf_serializer_get_description, "resultFormat");
  sd_add_format_descriptions(sd_model, service_node, librdf_query_results_formats_get_description, "resultFormat");
  sd_add_query_languages(sd_model, service_node);

  librdf_model_add(sd_model,
                   librdf_new_node_from_node(service_node),
                   librdf_new_node_from_node(LIBRDF_S_label(world)),
                   librdf_new_node_from_literal(world, (unsigned char *) storage_name, NULL, 0)
      );

  #define COMMENT_MAX_LEN   (128)
  comment = malloc(COMMENT_MAX_LEN);
  snprintf(comment, COMMENT_MAX_LEN, "RedStore %s endpoint using the '%s' storage module.",
           PACKAGE_VERSION, storage_type);
  librdf_model_add(sd_model,
                   librdf_new_node_from_node(service_node),
                   librdf_new_node_from_node(LIBRDF_S_comment(world)),
                   librdf_new_node_from_literal(world, (unsigned char *) comment, NULL, 0)
      );
  free(comment);

  librdf_model_add(sd_model,
                   librdf_new_node_from_node(service_node),
                   librdf_new_node_from_uri_local_name(world, sd_ns_uri, (unsigned char *) "endpoint"),
                   sd_get_endpoint_node(request_url)
      );

  librdf_free_node(service_node);

  return sd_model;
}



static void description_html_table(const char *title, description_proc_t desc_proc, redhttp_response_t * response)
{
  int i,n;

  redstore_page_append_strings(response, "<h2>", title, "</h2>\n", NULL);
  redstore_page_append_string(response, "<table border=\"1\">\n");
  redstore_page_append_string(response,
                              "<tr><th>Name</th><th>Description</th><th>MIME Type</th><th>Spec</th></tr>\n");

  for(i=0; 1; i++) {
    const raptor_syntax_description* desc = desc_proc(world, i);
    if (!desc)
      break;

    redstore_page_append_string(response, "<tr>");

    redstore_page_append_string(response, "<td>");
    for (n = 0; desc->names[n]; n++) {
      redstore_page_append_strings(response, desc->names[n], "<br />\n", NULL);
    }
    redstore_page_append_string(response, "</td>\n");

    redstore_page_append_string(response, "<td>");
    if (desc->label)
      redstore_page_append_string(response, desc->label);
    redstore_page_append_string(response, "</td>\n");

    redstore_page_append_string(response, "<td>");
    for (n = 0; n < desc->mime_types_count; n++) {
      const raptor_type_q mime_type = desc->mime_types[n];
      redstore_page_append_string(response, mime_type.mime_type);

      if (mime_type.q != 10) {
        redstore_page_append_string(response, " <i>(");
        redstore_page_append_decimal(response, mime_type.q / 10);
        redstore_page_append_string(response, ".");
        redstore_page_append_decimal(response, mime_type.q % 10);
        redstore_page_append_string(response, ")</i>");
      }
      redstore_page_append_string(response, "<br />\n");
    }
    redstore_page_append_string(response, "</td>\n");

    redstore_page_append_string(response, "<td>");
    if (desc->uri_strings) {
      for (n = 0; desc->uri_strings[n]; n++) {
        const char *uri_string = desc->uri_strings[n];
        redstore_page_append_string(response, "<a href=\"");
        redstore_page_append_escaped(response, uri_string, '"');
        redstore_page_append_string(response, "\">");
        redstore_page_append_escaped(response, uri_string, 0);
        redstore_page_append_string(response, "</a>");
        redstore_page_append_string(response, "<br />\n");
      }
    }
    redstore_page_append_string(response, "</td>\n");

    redstore_page_append_string(response, "</tr>\n");
  }
  redstore_page_append_string(response, "</table>\n");
}

static redhttp_response_t *handle_html_description(redhttp_request_t * request, void *user_data)
{

  redhttp_response_t *response = redstore_page_new(REDHTTP_OK, "Service Description");

  redstore_page_append_string(response, "<h2>Store Information</h2>\n");
  redstore_page_append_string(response, "<table border=\"1\">\n");
  redstore_page_append_strings(response, "<tr><th>Storage Name</th><td>", storage_name, "</td></tr>\n", NULL);
  redstore_page_append_strings(response, "<tr><th>Storage Type</th><td>", storage_type, "</td></tr>\n", NULL);
  redstore_page_append_strings(response, "<tr><th>Storage Options</th><td>", public_storage_options, "</td></tr>\n", NULL);

  redstore_page_append_string(response, "<tr><th>Triple Count</th><td>");
  redstore_page_append_decimal(response, librdf_storage_size(storage));
  redstore_page_append_string(response, "</td></tr>\n");

  redstore_page_append_string(response, "<tr><th>Named Graph Count</th><td>");
  redstore_page_append_decimal(response, context_count(storage));
  redstore_page_append_string(response, "</td></tr>\n");

  redstore_page_append_string(response, "<tr><th>HTTP Request Count</th><td>");
  redstore_page_append_decimal(response, request_count);
  redstore_page_append_string(response, "</td></tr>\n");

  redstore_page_append_string(response, "<tr><th>Successful Import Count</th><td>");
  redstore_page_append_decimal(response, import_count);
  redstore_page_append_string(response, "</td></tr>\n");

  redstore_page_append_string(response, "<tr><th>SPARQL Query Count</th><td>");
  redstore_page_append_decimal(response, query_count);
  redstore_page_append_string(response, "</td></tr>\n");
  redstore_page_append_string(response, "</table>\n");

  description_html_table("Query Languages", librdf_query_language_get_description, response);
  description_html_table("Query Result Formats", librdf_query_results_formats_get_description, response);
  description_html_table("Input RDF Formats", librdf_parser_get_description, response);
  description_html_table("Output RDF Formats", librdf_serializer_get_description, response);

  redstore_page_append_string(response,
                              "<p>This document is also available as "
                              "<a href=\"description?format=turtle\">RDF</a>.</p>\n");


  redstore_page_end(response);

  return response;
}

redhttp_response_t *handle_description_get(redhttp_request_t * request, void *user_data)
{
  char *format_str = redstore_get_format(request, accepted_serialiser_types, "text/html");
  redhttp_response_t *response = NULL;
  librdf_storage *sd_storage = NULL;
  librdf_model *sd_model = NULL;
  librdf_stream *sd_stream = NULL;

  if (format_str == NULL || redstore_is_html_format(format_str)) {
    response = handle_html_description(request, user_data);
  } else {
    const char *request_url = redhttp_request_get_url(request);

    sd_storage = librdf_new_storage(world, NULL, NULL, NULL);
    if (!sd_storage) {
      redstore_error("Failed to create temporary storage for service description.");
      goto CLEANUP;
    }

    sd_model = create_service_description(sd_storage, request_url);
    if (!sd_model) {
      redstore_error("Failed to create model for service description.");
      goto CLEANUP;
    }

    sd_stream = librdf_model_as_stream(sd_model);
    if (!sd_stream) {
      redstore_error("Failed to create stream for service description.");
      goto CLEANUP;
    }

    response = format_graph_stream(request, sd_stream);
    if (!response) {
      redstore_error("Failed to create temporary storage for service description.");
      goto CLEANUP;
    }
  }


CLEANUP:
  if (sd_stream)
    librdf_free_stream(sd_stream);
  if (sd_model)
    librdf_free_model(sd_model);
  if (sd_storage)
    librdf_free_storage(sd_storage);
  if (format_str)
    free(format_str);

  return response;
}

int description_init()
{
  // Create namespace URIs
  format_ns_uri = librdf_new_uri(world, (unsigned char *) "http://www.w3.org/ns/formats/");
  if (!format_ns_uri) {
    redstore_error("Failed to initialise format_ns_uri");
    return 1;
  }

  sd_ns_uri = librdf_new_uri(world, (unsigned char *) "http://www.w3.org/ns/sparql-service-description#");
  if (!sd_ns_uri) {
    redstore_error("Failed to initialise sd_ns_uri");
    return 1;
  }

  // Success
  return 0;
}

void description_free()
{
  if (format_ns_uri)
    librdf_free_uri(format_ns_uri);

  if (sd_ns_uri)
    librdf_free_uri(sd_ns_uri);
}

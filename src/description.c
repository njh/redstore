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
#include <assert.h>

#include "redstore.h"


static librdf_uri *saddle_ns_uri = NULL;
static librdf_uri *sd_ns_uri = NULL;

static librdf_storage *sd_storage = NULL;
static librdf_model *sd_model = NULL;
static librdf_node *service_node = NULL;


static int description_add_query_languages()
{
    rasqal_world *rasqal_world = rasqal_new_world();
    int i;

    // FIXME: This should use the librdf API
    for (i = 0; 1; i++) {
        const char *name, *label;
        const unsigned char *uri;
        librdf_node *bnode = NULL;

        if (rasqal_languages_enumerate(rasqal_world, i, &name, &label, &uri))
            break;

        bnode = librdf_new_node_from_blank_identifier(world, NULL);
        librdf_model_add(sd_model,
                         librdf_new_node_from_node(service_node),
                         librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char*)"queryLanguage"),
                         librdf_new_node_from_node(bnode)
            );

        if (name) {
            librdf_model_add(sd_model,
                             librdf_new_node_from_node(bnode),
                             LIBRDF_S_label(world),
                             librdf_new_node_from_literal(world, (unsigned char*)name, NULL, 0)
                );
        }

        if (label) {
            librdf_model_add(sd_model,
                             librdf_new_node_from_node(bnode),
                             LIBRDF_S_comment(world),
                             librdf_new_node_from_literal(world, (unsigned char*)label, NULL, 0)
                );
        }

        if (uri) {
            librdf_model_add(sd_model,
                             librdf_new_node_from_node(bnode),
                             librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char*)"spec"),
                             librdf_new_node_from_uri_string(world, uri)
                );
        }

        librdf_free_node(bnode);
    }

    rasqal_free_world(rasqal_world);

    return 0;
}


static int description_add_query_result_formats()
{
    int i;

    for (i = 0; 1; i++) {
        const char *name, *label, *mime_type;
        const unsigned char *uri;
        librdf_node *bnode = NULL;

        if (librdf_query_results_formats_enumerate(world, i, &name, &label, &uri, &mime_type))
            break;

        bnode = librdf_new_node_from_blank_identifier(world, NULL);
        librdf_model_add(sd_model,
                         librdf_new_node_from_node(service_node),
                         librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char*)"resultFormat"),
                         librdf_new_node_from_node(bnode)
            );

        if (name) {
            librdf_model_add(sd_model,
                             librdf_new_node_from_node(bnode),
                             LIBRDF_S_label(world),
                             librdf_new_node_from_literal(world, (unsigned char*)name, NULL, 0)
                );
        }

        if (label) {
            librdf_model_add(sd_model,
                             librdf_new_node_from_node(bnode),
                             LIBRDF_S_comment(world),
                             librdf_new_node_from_literal(world, (unsigned char*)label, NULL, 0)
                );
        }

        if (mime_type) {
            librdf_model_add(sd_model,
                             librdf_new_node_from_node(bnode),
                             librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char*)"mediaType"),
                             librdf_new_node_from_literal(world, (unsigned char*)mime_type, NULL, 0)
                );
        }

        if (uri) {
            librdf_model_add(sd_model,
                             librdf_new_node_from_node(bnode),
                             librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char*)"spec"),
                             librdf_new_node_from_uri_string(world, uri)
                );
        }

        librdf_free_node(bnode);
    }

    return 0;
}

static int description_add_serialisers()
{
    int i;

    // FIXME: This should use the librdf API
    for (i = 0; 1; i++) {
        const char *name, *label, *mime_type;
        const unsigned char *uri;
        librdf_node *bnode = NULL;

        if (raptor_serializers_enumerate(i, &name, &label, &mime_type, &uri))
            break;

        bnode = librdf_new_node_from_blank_identifier(world, NULL);
        librdf_model_add(sd_model,
                         librdf_new_node_from_node(service_node),
                         librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char*)"resultFormat"),
                         librdf_new_node_from_node(bnode)
            );

        if (name) {
            librdf_model_add(sd_model,
                             librdf_new_node_from_node(bnode),
                             LIBRDF_S_label(world),
                             librdf_new_node_from_literal(world, (unsigned char*)name, NULL, 0)
                );
        }

        if (label) {
            librdf_model_add(sd_model,
                             librdf_new_node_from_node(bnode),
                             LIBRDF_S_comment(world),
                             librdf_new_node_from_literal(world, (unsigned char*)label, NULL, 0)
                );
        }

        if (mime_type) {
            librdf_model_add(sd_model,
                             librdf_new_node_from_node(bnode),
                             librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char*)"mediaType"),
                             librdf_new_node_from_literal(world, (unsigned char*)mime_type, NULL, 0)
                );
        }

        if (uri) {
            librdf_model_add(sd_model,
                             librdf_new_node_from_node(bnode),
                             librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char*)"spec"),
                             librdf_new_node_from_uri_string(world, uri)
                );
        }

        librdf_free_node(bnode);
    }
    
    return 0;
}


int description_init(void)
{
    char *description;

    // Create namespace URIs
    saddle_ns_uri = librdf_new_uri(world, (unsigned char*)"http://www.w3.org/2005/03/saddle/#");
    sd_ns_uri = librdf_new_uri(world, (unsigned char*)"http://www.w3.org/ns/sparql-service-description#");

    sd_storage = librdf_new_storage(world, "memory", "service_description", "");
    if (!sd_storage) {
        perror("Failed to create storage for service description.");
        return -1;
    }

    sd_model = librdf_new_model(world, sd_storage, NULL);
    if (!sd_model) {
        perror("Failed to create model for service description.");
        return -1;
    }

    service_node = librdf_new_node_from_blank_identifier(world, NULL);
    librdf_model_add(sd_model,
                     librdf_new_node_from_node(service_node),
                     LIBRDF_MS_type(world),
                     librdf_new_node_from_uri_local_name(world, sd_ns_uri, (unsigned char*)"Service")
        );

    description_add_query_languages();
    description_add_serialisers();
    description_add_query_result_formats();

    librdf_model_add(sd_model,
                     librdf_new_node_from_node(service_node),
                     LIBRDF_S_label(world),
                     librdf_new_node_from_literal(world, (unsigned char*)storage_name, NULL, 0)
        );

    description = malloc(128);
    snprintf(description, 128, "RedStore %s endpoint using the '%s' storage module.",
             PACKAGE_VERSION, storage_type);
    librdf_model_add(sd_model,
                     librdf_new_node_from_node(service_node),
                     LIBRDF_S_comment(world),
                     librdf_new_node_from_literal(world, (unsigned char*)description, NULL, 0)
        );
    free(description);

    return 0;
}


static librdf_node *new_node_from_integer(librdf_world * world, int i)
{
    librdf_uri *xsd_integer_uri = NULL;
    unsigned char *string = NULL;

#define INTEGER_BUFFER_SIZE 20
    string = (unsigned char *) malloc(INTEGER_BUFFER_SIZE + 1);
    if (!string)
        return NULL;

    /* snprintf() takes as length the buffer size including NUL */
    snprintf((char *) string, INTEGER_BUFFER_SIZE + 1, "%d", i);

    xsd_integer_uri = librdf_new_uri(world, (unsigned char*)"http://www.w3.org/2001/XMLSchema#integer");
    return librdf_new_node_from_typed_literal(world, string, NULL, xsd_integer_uri);
}

static int model_remove_statements(librdf_model * model, librdf_statement * statement)
{
    librdf_stream *stream = librdf_model_find_statements(model, statement);
    int count = 0;

    while (stream && !librdf_stream_end(stream)) {
        librdf_statement *match = librdf_stream_get_object(stream);
        librdf_model_remove_statement(model, match);
        librdf_stream_next(stream);
        count++;
    }

    librdf_free_stream(stream);
    return count;
}

int description_update(void)
{
    librdf_statement *statement = librdf_new_statement_from_nodes(world,
                                                                  librdf_new_node_from_node
                                                                  (service_node),
                                                                  librdf_new_node_from_uri_local_name
                                                                  (world, sd_ns_uri,
                                                                  (unsigned char*)"totalTriples"),
                                                                  NULL);

    model_remove_statements(sd_model, statement);

    librdf_statement_set_object(statement,
                                new_node_from_integer(world, librdf_storage_size(storage)));

    librdf_model_add_statement(sd_model, statement);
    
    return 0;
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

static int model_write_target_cell(librdf_node * source, librdf_node * arc,
                                   redhttp_response_t * response)
{
    librdf_node *node = librdf_model_get_target(sd_model, source, arc);
    unsigned char *str;

    if (node) {
        if (librdf_node_is_literal(node)) {
            str = librdf_node_get_literal_value(node);
            redhttp_response_content_append(response, "<td>%s</td>", str);
        } else if (librdf_node_is_resource(node)) {
            str = librdf_uri_as_string(librdf_node_get_uri(node));
            redhttp_response_content_append(response, "<td><a href=\"%s\">%s</a></td>", str, str);
        } else if (librdf_node_is_blank(node)) {
            str = librdf_node_get_blank_identifier(node);
            redhttp_response_content_append(response, "<td>_:%s</td>", str);
        }
    } else {
        redhttp_response_content_append(response, "<td>&nbsp;</td>");
    }
    
    return 0;
}

static redhttp_response_t *handle_html_description(redhttp_request_t * request, void *user_data)
{
    redhttp_response_t *response = redhttp_response_new(REDHTTP_OK, NULL);
    librdf_node *rf_node =
        librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char*)"resultFormat");
    librdf_node *ql_node =
        librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char*)"queryLanguage");
    librdf_node *mt_node = librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char*)"mediaType");
    librdf_node *spec_node = librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char*)"spec");
    librdf_node *total_node = librdf_new_node_from_uri_local_name(world, sd_ns_uri, (unsigned char*)"totalTriples");
    librdf_iterator *iterator;

    page_append_html_header(response, "Service Description");

    redhttp_response_content_append(response, "<h2>Store Information</h2>\n");
    redhttp_response_content_append(response, "<table border=\"1\">\n");
    redhttp_response_content_append(response, "<tr><th>Store Name</th>");
    model_write_target_cell(service_node, LIBRDF_S_label(world), response);
    redhttp_response_content_append(response, "</tr>\n");
    redhttp_response_content_append(response, "<tr><th>Description</th>");
    model_write_target_cell(service_node, LIBRDF_S_comment(world), response);
    redhttp_response_content_append(response, "</tr>\n");
    redhttp_response_content_append(response, "<tr><th>Triple Count</th>");
    model_write_target_cell(service_node, total_node, response);
    redhttp_response_content_append(response, "</tr>\n");
    // FIXME: Put these into the RDF:
    redhttp_response_content_append(response,
                                    "<tr><th>Named Graph Count</th><td>%d</td></tr>\n",
                                    context_count(storage));
    redhttp_response_content_append(response,
                                    "<tr><th>HTTP Request Count</th><td>%lu</td></tr>\n",
                                    request_count);
    redhttp_response_content_append(response,
                                    "<tr><th>Successful Import Count</th><td>%lu</td></tr>\n",
                                    import_count);
    redhttp_response_content_append(response, "<tr><th>SPARQL Query Count</th><td>%lu</td></tr>\n",
                                    query_count);
    redhttp_response_content_append(response, "</table>\n");

    redhttp_response_content_append(response, "<h2>Query Languages</h2>\n");
    redhttp_response_content_append(response, "<table border=\"1\">\n");
    redhttp_response_content_append(response,
                                    "<tr><th>Name</th><th>Description</th><th>Spec</th></tr>\n");
    iterator = librdf_model_get_targets(sd_model, service_node, ql_node);
    while (!librdf_iterator_end(iterator)) {
        librdf_node *format_node = librdf_iterator_get_object(iterator);
        if (format_node) {
            redhttp_response_content_append(response, "<tr>");
            model_write_target_cell(format_node, LIBRDF_S_label(world), response);
            model_write_target_cell(format_node, LIBRDF_S_comment(world), response);
            model_write_target_cell(format_node, spec_node, response);
            redhttp_response_content_append(response, "</tr>\n");
        }
        librdf_iterator_next(iterator);
    }
    librdf_free_iterator(iterator);
    redhttp_response_content_append(response, "</table>\n");


    redhttp_response_content_append(response, "<h2>Result Formats</h2>\n");
    redhttp_response_content_append(response, "<table border=\"1\">\n");
    redhttp_response_content_append(response,
                                    "<tr><th>Name</th><th>Description</th><th>MIME Type</th><th>Spec</th></tr>\n");
    iterator = librdf_model_get_targets(sd_model, service_node, rf_node);
    while (!librdf_iterator_end(iterator)) {
        librdf_node *format_node = librdf_iterator_get_object(iterator);
        if (format_node) {
            redhttp_response_content_append(response, "<tr>");
            model_write_target_cell(format_node, LIBRDF_S_label(world), response);
            model_write_target_cell(format_node, LIBRDF_S_comment(world), response);
            model_write_target_cell(format_node, mt_node, response);
            model_write_target_cell(format_node, spec_node, response);
            redhttp_response_content_append(response, "</tr>\n");
        }
        librdf_iterator_next(iterator);
    }
    librdf_free_iterator(iterator);
    redhttp_response_content_append(response, "</table>\n");

    redhttp_response_content_append(response,
                                    "<p>This document is also available as "
                                    "<a href=\"description?format=turtle\">RDF</a>.</p>\n");

    page_append_html_footer(response);

    // FIXME: free nodes?

    return response;
}

redhttp_response_t *handle_description_get(redhttp_request_t * request, void *user_data)
{
    const char *format_str = get_format(request);

    description_update();

    if (format_str == NULL ||
        strcmp(format_str, "html") == 0 ||
        strcmp(format_str, "application/xml") == 0 ||
        strcmp(format_str, "application/xhtml+xml") == 0 || strcmp(format_str, "text/html") == 0) {
        return handle_html_description(request, user_data);
    } else {
        return format_graph_stream_librdf(request, librdf_model_as_stream(sd_model), format_str);
    }
}

void description_free(void)
{
    librdf_free_storage(sd_storage);

    librdf_free_uri(saddle_ns_uri);
    librdf_free_uri(sd_ns_uri);
}

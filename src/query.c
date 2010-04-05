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

static redhttp_response_t *perform_query(redhttp_request_t * request, const char *query_string)
{
    librdf_query *query = NULL;
    librdf_query_results *results = NULL;
    redhttp_response_t *response = NULL;
    const char *lang = redhttp_request_get_argument(request, "lang");

    if (lang == NULL)
        lang = DEFAULT_QUERY_LANGUAGE;

    redstore_debug("query_lang='%s'", lang);
    redstore_debug("query_string='%s'", query_string);

    query = librdf_new_query(world, lang, NULL, (unsigned char *) query_string, NULL);
    if (!query) {
        response =
            redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                "librdf_new_query failed");
        goto CLEANUP;
    }

    results = librdf_model_query_execute(model, query);
    if (!results) {
        response =
            redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR,
                                "librdf_model_query_execute failed");
        goto CLEANUP;
    }

    query_count++;

    if (librdf_query_results_is_bindings(results)) {
        response = format_bindings_query_result(request, results);
    } else if (librdf_query_results_is_graph(results)) {
        librdf_stream *stream = librdf_query_results_as_stream(results);
        response = format_graph_stream(request, stream);
        librdf_free_stream(stream);
    } else if (librdf_query_results_is_boolean(results)) {
        response = format_bindings_query_result(request, results);
    } else if (librdf_query_results_is_syntax(results)) {
        response =
            redstore_error_page(REDSTORE_INFO, REDHTTP_NOT_IMPLEMENTED,
                                "Syntax results format is not supported.");
    } else {
        response =
            redstore_error_page(REDSTORE_INFO, REDHTTP_INTERNAL_SERVER_ERROR,
                                "Unknown results type.");
    }


  CLEANUP:
    if (results)
        librdf_free_query_results(results);
    if (query)
        librdf_free_query(query);

    return response;
}


static void syntax_select_list(const char *name, const char *default_name, librdf_node * source,
                               librdf_node * arc, redhttp_response_t * response)
{
    librdf_iterator *iterator;

    redstore_page_append_strings(response, "<select name=\"", name, "\">\n", NULL);
    iterator = librdf_model_get_targets(sd_model, source, arc);
    while (!librdf_iterator_end(iterator)) {
        librdf_node *format_node = librdf_iterator_get_object(iterator);
        if (format_node) {
            librdf_node *name =
                librdf_model_get_target(sd_model, format_node, LIBRDF_S_label(world));
            librdf_node *desc =
                librdf_model_get_target(sd_model, format_node, LIBRDF_S_comment(world));
            if (name && desc) {
                const char *name_str = (char *) librdf_node_get_literal_value(name);
                redstore_page_append_strings(response, "<option value=\"", name_str, "\"", NULL);
                if (strcmp(name_str, default_name) == 0)
                    redstore_page_append_string(response, " selected=\"selected\"");
                redstore_page_append_string(response, ">");
                redstore_page_append_escaped(response, (char *) librdf_node_get_literal_value(desc),
                                             0);
                redstore_page_append_string(response, "</option>\n");
            }
        }
        librdf_iterator_next(iterator);
    }
    librdf_free_iterator(iterator);
    redstore_page_append_string(response, "</select>\n");
}


redhttp_response_t *handle_query(redhttp_request_t * request, void *user_data)
{
    librdf_node *ql_node = librdf_new_node_from_uri_local_name(world, saddle_ns_uri,
                                                               (unsigned char *) "queryLanguage");
    librdf_node *rf_node =
        librdf_new_node_from_uri_local_name(world, saddle_ns_uri, (unsigned char *) "resultFormat");
    redhttp_response_t *response = NULL;
    const char *query_string = NULL;

    // Do we have a query string?
    query_string = redhttp_request_get_argument(request, "query");
    if (query_string) {
        response = perform_query(request, query_string);
    } else {
        response = redstore_page_new("Query");
        redstore_page_append_string(response, "<form action=\"./query\" method=\"get\">\n");
        redstore_page_append_string(response,
                                    "<div><textarea name=\"query\" cols=\"80\" rows=\"18\">\n");
        redstore_page_append_string(response,
                                    "PREFIX rdf: &lt;http://www.w3.org/1999/02/22-rdf-syntax-ns#&gt;\n");
        redstore_page_append_string(response,
                                    "PREFIX rdfs: &lt;http://www.w3.org/2000/01/rdf-schema#&gt;\n\n");
        redstore_page_append_string(response, "SELECT * WHERE {\n ?s ?p ?o\n} LIMIT 10\n");
        redstore_page_append_string(response, "</textarea></div>\n");

        redstore_page_append_string(response, "<div class=\"buttons\">\n");
        redstore_page_append_string(response, "Query Language: ");
        syntax_select_list("lang", DEFAULT_QUERY_LANGUAGE, service_node, ql_node, response);
        redstore_page_append_string(response, "<br />");

        redstore_page_append_string(response, "Result Format: ");
        syntax_select_list("format", "html", service_node, rf_node, response);
        redstore_page_append_string(response, "<br />");

        redstore_page_append_string(response, "<input type=\"reset\" /> ");
        redstore_page_append_string(response, "<input type=\"submit\" value=\"Execute\" />\n");
        redstore_page_append_string(response, "</div></form>\n");
        redstore_page_end(response);
    }

    librdf_free_node(ql_node);
    librdf_free_node(rf_node);

    return response;
}

redhttp_response_t *handle_sparql(redhttp_request_t * request, void *user_data)
{
    redhttp_response_t *response = NULL;
    const char *query_string = NULL;

    query_string = redhttp_request_get_argument(request, "query");
    if (query_string) {
        response = perform_query(request, query_string);
    } else {
        response = redstore_error_page(REDSTORE_INFO, REDHTTP_BAD_REQUEST, "Missing query string.");
    }

    return response;
}

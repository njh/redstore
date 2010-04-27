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
        if (stream) {
            response = format_graph_stream(request, stream);
            librdf_free_stream(stream);
        } else {
            response =
                redstore_error_page(REDSTORE_DEBUG, REDHTTP_INTERNAL_SERVER_ERROR,
                                    "Failed to get query results graph.");
        }
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




redhttp_response_t *handle_query(redhttp_request_t * request, void *user_data)
{
    const char *query_string = NULL;

    // Do we have a query string?
    query_string = redhttp_request_get_argument(request, "query");
    if (query_string) {
        return perform_query(request, query_string);
    } else {
        return handle_page_query_form(request, user_data);
    }
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

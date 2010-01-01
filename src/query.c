#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <librdf.h>

#include "redstore.h"


http_response_t* handle_sparql_query(http_request_t *request)
{
    char *query_string = NULL;
    librdf_query* query;
    librdf_query_results* results;
    http_response_t* response;

    query_string = http_get_argument(request, "query");
    if (!query_string) {
        // FIXME: there must be a better response to a missing parameter?
        redstore_debug("query was missing query string");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    query = librdf_new_query(world, "sparql", NULL, (unsigned char *)query_string, NULL);
    if (!query) {
        free(query_string);
        redstore_error("librdf_new_query failed");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    results = librdf_model_query_execute(model, query);
    if (!results) {
        free(query_string);
        librdf_free_query(query);
        redstore_error("librdf_storage_query_execute failed");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    if (librdf_query_results_is_bindings(results)) {
        response = format_bindings_query_result(request, results);
    } else if (librdf_query_results_is_graph(results)) {
        librdf_stream *stream = librdf_query_results_as_stream(results);
        response = format_graph_stream(request, stream);
    } else if (librdf_query_results_is_boolean(results)) {
        redstore_info("librdf_query_results_is_boolean not supported");
        return handle_error(request, MHD_HTTP_NOT_IMPLEMENTED);
    } else if (librdf_query_results_is_syntax(results)) {
        redstore_info("librdf_query_results_is_syntax not supported");
        return handle_error(request, MHD_HTTP_NOT_IMPLEMENTED);
    } else {
        redstore_error("unknown query result type");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    librdf_free_query_results(results);
    librdf_free_query(query);
    free(query_string);

    return response;
}

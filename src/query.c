#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "redstore.h"


http_response_t* handle_sparql_query(http_request_t *request, void* user_data)
{
    char *query_string = NULL;
    librdf_query* query = NULL;
    librdf_query_results* results = NULL;
    http_response_t* response = NULL;

    query_string = http_request_get_argument(request, "query");
    if (!query_string) {
        // FIXME: there must be a better response to a missing parameter?
        redstore_debug("query was missing query string");
        response = http_response_new_error_page(400, "query was missing query string");
        goto CLEANUP;
    }

    query = librdf_new_query(world, "sparql", NULL, (unsigned char *)query_string, NULL);
    if (!query) {
        redstore_error("librdf_new_query failed");
        response = http_response_new_error_page(500, "librdf_new_query failed");
        goto CLEANUP;
    }

    results = librdf_model_query_execute(model, query);
    if (!results) {
        redstore_error("librdf_storage_query_execute failed");
        response = http_response_new_error_page(500, "librdf_storage_query_execute failed");
        goto CLEANUP;
    }
    
    query_count++;

    if (librdf_query_results_is_bindings(results)) {
        response = format_bindings_query_result(request, results);
    } else if (librdf_query_results_is_graph(results)) {
        librdf_stream *stream = librdf_query_results_as_stream(results);
        response = format_graph_stream(request, stream);
    } else if (librdf_query_results_is_boolean(results)) {
        redstore_info("librdf_query_results_is_boolean not supported");
        return http_response_new_error_page(501, NULL);
    } else if (librdf_query_results_is_syntax(results)) {
        redstore_info("librdf_query_results_is_syntax not supported");
        return http_response_new_error_page(501, NULL);
    } else {
        redstore_error("unknown query result type");
        return http_response_new_error_page(500, NULL);
    }


CLEANUP:
    if (results) librdf_free_query_results(results);
    if (query) librdf_free_query(query);
    if (query_string) free(query_string);

    return response;
}

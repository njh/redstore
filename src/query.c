#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <librdf.h>

#include "redstore.h"

static http_response_t* format_results_librdf(http_request_t *request, librdf_query_results* results, const char* format_str)
{
    librdf_uri *format_uri = NULL;
    const char *mime_type = NULL;
    char* data = NULL;
    unsigned int i;
    
    for(i=0; 1; i++) {
      const char *name;
      const unsigned char *uri;
      const char *mime;
      if(librdf_query_results_formats_enumerate(world, i, 
                                                &name, NULL, 
                                                &uri, &mime))
          break;
      if (strcmp(format_str, name)==0 ||
          strcmp(format_str, (char*)uri)==0 ||
          strcmp(format_str, mime)==0)
      {
          format_uri = librdf_new_uri(world,uri);
          mime_type = mime;
      }
    }
    
    if (!format_uri) {
        redstore_error("Failed to match file format");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    // FIXME: stream results back to client, rather than building string
    data = (char*)librdf_query_results_to_string(results, format_uri, NULL);
    if (!data) {
        redstore_error("Failed to serialise query results");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    librdf_free_uri(format_uri);

    return new_http_response(request, MHD_HTTP_OK, data, strlen(data), mime_type);
}

static http_response_t* format_results_html(http_request_t *request, librdf_query_results* results)
{
    FILE* stream = NULL;
    char *string_buffer = NULL;
    size_t string_size = 0;
    int i, count;

    stream = open_memstream(&string_buffer, &string_size);
    if(!stream) {
        redstore_error("Failed to open open_memstream");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    count = librdf_query_results_get_bindings_count(results);
    fprintf(stream, "<table class=\"sparql\" border=\"1\">\n<tr>");
    for(i=0; i < count; i++) {
        const char* name = librdf_query_results_get_binding_name(results, i);
        fprintf(stream, "<th>?%s</th>", name);
    }
    fprintf(stream, "</tr>\n");

    while(!librdf_query_results_finished(results)) {
        fprintf(stream, "<tr>");
        for(i=0; i < count; i++) {
            librdf_node *value = librdf_query_results_get_binding_value(results, i);
            fprintf(stream, "<td>");
            if (value) {
                librdf_node_print(value, stream);
                librdf_free_node(value);
            } else {
                fputs("NULL", stream);
            }
            fprintf(stream, "</td>");
        }
        fprintf(stream, "</tr>\n");
        librdf_query_results_next(results);
    }

    fprintf(stream, "</table>\n");
    fclose(stream);
   
    return handle_html_page(request, MHD_HTTP_OK, "SPARQL Results", string_buffer);
}

static http_response_t* format_results_text(http_request_t *request, librdf_query_results* results)
{
    FILE* stream = NULL;
    char *string_buffer = NULL;
    size_t string_size = 0;
    int i, count;

    stream = open_memstream(&string_buffer, &string_size);
    if(!stream) {
        redstore_error("Failed to open open_memstream");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    count = librdf_query_results_get_bindings_count(results);
    for(i=0; i < count; i++) {
        const char* name = librdf_query_results_get_binding_name(results, i);
        fprintf(stream, "?%s", name);
        if (i!=count-1) fprintf(stream, "\t");
    }
    fprintf(stream, "\n");

    while(!librdf_query_results_finished(results)) {
        for(i=0; i < count; i++) {
            librdf_node *value = librdf_query_results_get_binding_value(results, i);
            if (value) {
                char* str = (char*)librdf_node_to_string(value);
                if (str) {
                    fputs(str, stream);
                    free(str);
                }
                librdf_free_node(value);
            } else {
                fputs("NULL", stream);
            }
            if (i!=count-1) fprintf(stream, "\t");
        }
        fprintf(stream, "\n");
        librdf_query_results_next(results);
    }

    fclose(stream);

    return new_http_response(request, MHD_HTTP_OK, string_buffer, string_size, "text/plain");
}

http_response_t* handle_sparql_query(http_request_t *request)
{
    const char *query_string = NULL;
    const char *format = NULL;
    librdf_query* query;
    librdf_query_results* results;
    http_response_t* response;

    if (strcmp(request->method, MHD_HTTP_METHOD_GET)==0) {
        query_string = MHD_lookup_connection_value (request->connection, MHD_GET_ARGUMENT_KIND, "query");
        format = MHD_lookup_connection_value (request->connection, MHD_GET_ARGUMENT_KIND, "format");
    } else if (strcmp(request->method, MHD_HTTP_METHOD_POST)==0) {
        // FIXME: implement this
        return handle_error(request, MHD_HTTP_NOT_IMPLEMENTED);
    } else {
        // FIXME: MUST return the methods that are allowed
        return handle_error(request, MHD_HTTP_METHOD_NOT_ALLOWED);
    }

    if (!query_string) {
        // FIXME: there must be a better response to a missing parameter?
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    

    query = librdf_new_query(world, "sparql", NULL, (unsigned char *)query_string, NULL);
    if (!query) {
        redstore_error("librdf_new_query failed");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    results = librdf_model_query_execute(model, query);
    if (!results) {
        librdf_free_query(query);
        redstore_error("librdf_storage_query_execute failed");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    if (format == NULL) format = DEFAULT_RESULTS_FORMAT;
    if (strcmp(format, "html")==0) {
        response = format_results_html(request, results);
    } else if (strcmp(format, "text")==0) {
        response = format_results_text(request, results);
    } else {
        response = format_results_librdf(request, results, format);
    }

    redstore_debug("Query returned %d results", librdf_query_results_get_count(results));

    librdf_free_query_results(results);
    librdf_free_query(query);

    return response;
}

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <librdf.h>

#include "redstore.h"


http_response_t* format_graph_stream_librdf(http_request_t *request, librdf_stream* stream, const char* format_str)
{
    librdf_serializer* serialiser;
    const char* format_name = NULL;
    const char* mime_type = NULL;
    char* data = NULL;
    int i;
    
    for(i=0; serialiser_info[i].name; i++) {
        if (strcmp(format_str, serialiser_info[i].name)==0 ||
            strcmp(format_str, serialiser_info[i].uri)==0 ||
            strcmp(format_str, serialiser_info[i].mime_type)==0)
        {
            format_name = serialiser_info[i].name;
            mime_type = serialiser_info[i].mime_type;
        }
    }
    
    if (!format_name) {
        redstore_error("Failed to match file format: %s", format_str);
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    serialiser = librdf_new_serializer(world, format_name, NULL, NULL);
    if (!serialiser) {
        redstore_error("Failed to create serialiser for: %s", format_name);
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    data = (char*)librdf_serializer_serialize_stream_to_string(serialiser, NULL, stream);
    if (!data) {
        redstore_error("Failed to serialize graph");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    librdf_free_serializer(serialiser);

    return new_http_response(request, MHD_HTTP_OK, data, strlen(data), mime_type);
}


http_response_t* format_graph_stream_html(http_request_t *request, librdf_stream* stream, const char* format_str)
{
    FILE* iostream = NULL;
    char *string_buffer = NULL;
    size_t string_size = 0;

    iostream = open_memstream(&string_buffer, &string_size);
    if(!iostream) {
        redstore_error("Failed to open open_memstream");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    fprintf(iostream, "<table class=\"triples\" border=\"1\">\n");
    fprintf(iostream, "<tr><th>Subject</th><th>Predicate</th><th>Object</th></tr>\n");
    while(!librdf_stream_end(stream)) {
        librdf_statement *statement = librdf_stream_get_object(stream);
        if(!statement) {
            fprintf(stderr, "librdf_stream_next returned NULL\n");
            break;
        }
      
        fprintf(iostream, "<tr><td>");
        librdf_node_print(librdf_statement_get_subject(statement), iostream);
        fprintf(iostream, "</td><td>");
        librdf_node_print(librdf_statement_get_predicate(statement), iostream);
        fprintf(iostream, "</td><td>");
        librdf_node_print(librdf_statement_get_object(statement), iostream);
        fprintf(iostream, "</td></tr>\n");

        librdf_stream_next(stream);
    }

    fprintf(iostream, "</table>\n");
    fclose(iostream);
   
    return handle_html_page(request, MHD_HTTP_OK, "Graph Contents", string_buffer);
}


/*
  Return the first item in the accept header
  FIXME: do content negotiation properly
*/
static char* parse_accept_header(http_request_t *request)
{
    const char* accept_str = MHD_lookup_connection_value(request->connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_ACCEPT);
    int pos=-1, i;
    
    if (accept_str == NULL) return NULL;

    for(i=0; i<=strlen(accept_str); i++) {
        if (accept_str[i]=='\0' || 
            accept_str[i]==' ' ||
            accept_str[i]==',' ||
            accept_str[i]==';' )
        {
            pos = i;
            break;
        }
    }
    
    if (pos>0) {
        char* result = malloc(pos+1);
        strncpy(result,accept_str,pos);
        result[pos] = '\0';
        return result;
    } else {
        return NULL;
    }
}


http_response_t* format_graph_stream(http_request_t *request, librdf_stream* stream, const char* format_str)
{
    http_response_t* response;
    char *accept;

    if (format_str) {
      accept = strdup(format_str);
    } else {
      // Work out what format to return it as
      accept = parse_accept_header(request);
      redstore_debug("accept: %s", accept);
    }
    
    if (accept == NULL ||
        strcmp(accept, "html")==0 ||
        strcmp(accept, "application/xml")==0 ||
        strcmp(accept, "text/html")==0 )
    {
        response = format_graph_stream_html(request, stream, accept);
    } else {
        response = format_graph_stream_librdf(request, stream, accept);
    }

    if (accept) free(accept);
    
    return response;
}


http_response_t* format_bindings_query_result_librdf(http_request_t *request, librdf_query_results* results, const char* format_str)
{
    librdf_uri *format_uri = NULL;
    const char *mime_type = NULL;
    char* data = NULL;
    unsigned int i;
    
    for(i=0; 1; i++) {
        const char *name, *mime;
        const unsigned char *uri;
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
        redstore_error("Failed to match file format: %s", format_str);
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

http_response_t* format_bindings_query_result_html(http_request_t *request, librdf_query_results* results, const char* format_str)
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

http_response_t* format_bindings_query_result_text(http_request_t *request, librdf_query_results* results, const char* format_str)
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

http_response_t* format_bindings_query_result(http_request_t *request, librdf_query_results* results, const char* format_str)
{
    http_response_t* response;

    if (format_str == NULL) format_str = DEFAULT_RESULTS_FORMAT;
    if (strcmp(format_str, "html")==0) {
        response = format_bindings_query_result_html(request, results, format_str);
    } else if (strcmp(format_str, "text")==0) {
        response = format_bindings_query_result_text(request, results, format_str);
    } else {
        response = format_bindings_query_result_librdf(request, results, format_str);
    }

    redstore_debug("Query returned %d results", librdf_query_results_get_count(results));
    
    return response;
}



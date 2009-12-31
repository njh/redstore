
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <librdf.h>

#include "redstore.h"


http_response_t* handle_graph_index(http_request_t *request)
{
    librdf_iterator* iterator = NULL;
    FILE* stream = NULL;
    char *page_buffer = NULL;
    size_t page_size = 0;
 
    iterator = librdf_storage_get_contexts(storage);
    if(!iterator) {
        redstore_error("Failed to get contexts.");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    stream = open_memstream(&page_buffer, &page_size);
    if(!stream) {
        redstore_error("Failed to open open_memstream");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    fprintf(stream, "<ul>\n");
    while(!librdf_iterator_end(iterator)) {
        librdf_uri* uri;
        librdf_node* node;
        char* escaped;
        
        node = (librdf_node*)librdf_iterator_get_object(iterator);
        if(!node) {
            redstore_error("librdf_iterator_get_next returned NULL");
            break;
        }
        
        uri = librdf_node_get_uri(node);
        if(!uri) {
            redstore_error("Failed to get URI for context node");
            break;
        }
        
        escaped = escape_uri((char*)librdf_uri_as_string(uri));
        fprintf(stream, "<li><a href=\"/graphs/%s\">%s<a/></li>\n", escaped, librdf_uri_as_string(uri));
        free(escaped);
        
        librdf_iterator_next(iterator);
    }
    fprintf(stream, "</ul>\n");
    
    librdf_free_iterator(iterator);
    fclose(stream);
   
    return handle_html_page(request, MHD_HTTP_OK, "Named Graphs", page_buffer);
}


static http_response_t* format_stream_html(http_request_t *request, librdf_stream* stream)
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



static http_response_t* format_stream_librdf(
    http_request_t *request, librdf_stream* stream,
    const char* serialiser_name, const char* mime_type)
{
    librdf_serializer* serialiser;
    char* data = NULL;
   
    serialiser = librdf_new_serializer(world, serialiser_name, NULL, NULL);
    if (!serialiser) {
        redstore_error("Failed to create serialiser: %s", serialiser_name);
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


/*
  Return the first item in the accept header
  FIXME: do content negotiation properly
*/
char* parse_accept_header(http_request_t *request)
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


http_response_t* handle_graph_show(http_request_t *request, char* uri)
{
    librdf_node *context;
    librdf_stream * stream;
    http_response_t* response;
    char *accept;
    
    context = librdf_new_node_from_uri_string(world,(unsigned char*)uri);
    if (!context) return NULL;
    
    // First check if the graph exists
    if (!librdf_model_contains_context(model, context)) {
        redstore_debug("Graph not found: %s", uri);
        // FIXME: display a better error message
        return handle_error(request, MHD_HTTP_NOT_FOUND);
    }
    
    // Stream the graph
    stream = librdf_model_context_as_stream(model, context);
    if(!stream) {
        librdf_free_node(context);
        redstore_error("Failed to stream context.");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // Work out what format to return it as
    accept = parse_accept_header(request);
    redstore_debug("accept: %s", accept);
    if (!accept) accept = strdup("text/html");

    if (strcmp("application/rdf+xml", accept)==0) {
        response = format_stream_librdf(request, stream, "rdfxml-abbrev", accept);
    } else if (strcmp("application/x-turtle", accept)==0 || strcmp("text/turtle", accept)==0) {
        response = format_stream_librdf(request, stream, "turtle", accept);
    } else if (strcmp("application/x-ntriples", accept)==0) {
        response = format_stream_librdf(request, stream, "ntriples", accept);
    } else if (strcmp("application/json", accept)==0 || strcmp("text/json", accept)==0) {
        // FIXME: why doesn't this work?
        response = format_stream_librdf(request, stream, "json", accept);
    } else {
        // Default is to return HTML
        response = format_stream_html(request, stream);
    }
    
    librdf_free_stream(stream);
    free(uri);
    free(accept);
    
    return response;
}

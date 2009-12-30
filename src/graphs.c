
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
   
    return handle_html_page(request, MHD_HTTP_OK, "SPARQL Results", string_buffer);
}


http_response_t* handle_graph_show(http_request_t *request, char* uri)
{
    librdf_node *context;
    librdf_stream * stream;
    http_response_t* response;
    
    context = librdf_new_node_from_uri_string(world,(unsigned char*)uri);
    if (!context) return NULL;
    
    // First check if the graph exists
    if (!librdf_model_contains_context(model, context)) {
        redstore_error("Graph not found: %s", uri);
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

    response = format_stream_html(request, stream);

    librdf_free_stream(stream);
    free(uri);
    
    return response;
}

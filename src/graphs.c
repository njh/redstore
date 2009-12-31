
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


http_response_t* handle_graph_show(http_request_t *request, char* uri)
{
    librdf_node *context;
    librdf_stream * stream;
    http_response_t* response;
    
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
    
    response = format_graph_stream(request, stream, NULL);

    librdf_free_stream(stream);
    free(uri);
    
    return response;
}

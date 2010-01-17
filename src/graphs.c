#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "redstore.h"


http_response_t* handle_graph_index(http_request_t *request, void* user_data)
{
    http_response_t* response = http_response_new(200, NULL);
    librdf_iterator* iterator = NULL;

	page_append_html_header(response, "Named Graphs");
 
    iterator = librdf_storage_get_contexts(storage);
    if(!iterator) {
        redstore_error("Failed to get contexts.");
        return http_response_new_error_page(500, NULL);
    }

    http_response_content_append(response, "<ul>\n");
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
        
        escaped = http_url_escape((char*)librdf_uri_as_string(uri));
        http_response_content_append(response, "<li><a href=\"/data/%s\">%s<a/></li>\n", escaped, librdf_uri_as_string(uri));
        free(escaped);
        
        librdf_iterator_next(iterator);
    }
    http_response_content_append(response, "</ul>\n");
    
    librdf_free_iterator(iterator);

	page_append_html_footer(response);
	
	return response;
}


http_response_t* handle_graph_get(http_request_t *request, librdf_node *context)
{
    librdf_stream * stream;
    http_response_t* response;

    // First check if the graph exists
    if (!librdf_model_contains_context(model, context)) {
        unsigned char* uri = librdf_node_to_string(context);
        redstore_debug("Graph not found: %s", uri);
        free(uri);
        // FIXME: display a better error message
        return http_response_new_error_page(404, NULL);
    }
    
    // Stream the graph
    stream = librdf_model_context_as_stream(model, context);
    if(!stream) {
        librdf_free_node(context);
        redstore_error("Failed to stream context.");
        return http_response_new_error_page(500, NULL);
    }
    
    response = format_graph_stream(request, stream);

    librdf_free_stream(stream);

    return response;
}


http_response_t* handle_graph_delete(http_request_t *request, librdf_node *context)
{
    unsigned char* uri = librdf_node_to_string(context);
    http_response_t* response;

    // First check if the graph exists
    if (!librdf_model_contains_context(model, context)) {
        unsigned char* uri = librdf_node_to_string(context);
        redstore_debug("Graph not found: %s", uri);
        free(uri);
        // FIXME: display a better error message
        return http_response_new_error_page(404, NULL);
    }

    redstore_info("Deleting graph: %s", uri);
    
    if (librdf_model_context_remove_statements(model,context)) {
    	response = http_response_new_error_page(500, "Error while trying to delete graph");
    } else {
    	response = http_response_new_error_page(200, "Successfully deleted Graph");
    }
    
    free(uri);
    
    return response;
}

/*
http_response_t* handle_graph(http_request_t *request, void* user_data)
{
    http_response_t* response = NULL;
    librdf_node *context;
 
    redstore_debug("handle_graph(%s,%s)", request->method, uri);

    //response = http_allowed_methods(request, "GET", "DELETE");
    //if (response) return response;
    
    context = librdf_new_node_from_uri_string(world,(unsigned char*)uri);
    if (!context) return NULL;
    
    if (strcmp(request->method, "GET")==0) {
        response = handle_graph_get(request, context);
    } else if (strcmp(request->method, "DELETE")==0) {
        response = handle_graph_delete(request, context);
    }

    free(uri);
    librdf_free_node(context);
    
    return response;
}
*/
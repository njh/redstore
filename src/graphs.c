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

static librdf_node* get_graph_context(http_request_t *request)
{
	const char* uri = http_request_get_path_glob(request);
    librdf_node *context;

	if (!uri || strlen(uri)<1) {
		return NULL;
	}

	// Create node
	context = librdf_new_node_from_uri_string(world,(const unsigned char*)uri);
    if (!context) {
        redstore_debug("Failed to create node from: %s", uri);
    	return NULL;
    }

    // Check if the graph exists
    if (!librdf_model_contains_context(model, context)) {
        redstore_debug("Graph not found: %s", uri);
        // FIXME: display a better error message
        return NULL;
    }

	return context;
}

http_response_t* handle_graph_head(http_request_t *request, void* user_data)
{
    librdf_node *context = get_graph_context(request);
    http_response_t* response;

    if (context) {
    	response = http_response_new(200, NULL);
    	librdf_free_node(context);
    } else {
    	response = http_response_new(404, "Graph not found");
	}

    return response;
}

http_response_t* handle_graph_get(http_request_t *request, void* user_data)
{
    librdf_node *context = get_graph_context(request);
    librdf_stream * stream;
    http_response_t* response;

    if (!context) {
         return http_response_new_error_page(404, "Graph not found.");
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
    librdf_free_node(context);

    return response;
}


http_response_t* handle_graph_delete(http_request_t *request, void* user_data)
{
    librdf_node *context = get_graph_context(request);
    http_response_t* response;

	// Create node
    if (!context) {
         return http_response_new_error_page(404, "Graph not found.");
	}

    redstore_info("Deleting graph: %s", http_request_get_path_glob(request));
    
    if (librdf_model_context_remove_statements(model,context)) {
    	response = http_response_new_error_page(500, "Error while trying to delete graph");
    } else {
    	response = http_response_new_error_page(200, "Successfully deleted Graph");
    }
    
    librdf_free_node(context);
    
    return response;
}

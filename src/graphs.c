#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "redstore.h"


http_response_t* handle_graph_index(http_request_t *request, void* user_data)
{
    http_response_t* response = http_response_new(HTTP_OK, NULL);
    librdf_iterator* iterator = NULL;

	page_append_html_header(response, "Named Graphs");
 
    iterator = librdf_storage_get_contexts(storage);
    if(!iterator) {
        return redstore_error_page(REDSTORE_ERROR, HTTP_INTERNAL_SERVER_ERROR, "Failed to get list of graphs.");
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
    	response = http_response_new(HTTP_OK, NULL);
    	librdf_free_node(context);
    } else {
    	response = http_response_new(HTTP_NOT_FOUND, "Graph not found");
	}

    return response;
}

http_response_t* handle_graph_get(http_request_t *request, void* user_data)
{
    librdf_node *context = get_graph_context(request);
    librdf_stream * stream;
    http_response_t* response;

    if (!context) {
		return redstore_error_page(REDSTORE_INFO, HTTP_NOT_FOUND, "Graph not found.");
	}

    // Stream the graph
    stream = librdf_model_context_as_stream(model, context);
    if(!stream) {
        librdf_free_node(context);
        return redstore_error_page(REDSTORE_ERROR, HTTP_INTERNAL_SERVER_ERROR, "Failed to stream context.");
    }
    
    response = format_graph_stream(request, stream);

    librdf_free_stream(stream);
    librdf_free_node(context);

    return response;
}

http_response_t* handle_graph_put(http_request_t *request, void* user_data)
{
	const char* uri = http_request_get_path_glob(request);
	char* content_length = http_request_get_header(request, "content-length");
	char* content_type = http_request_get_header(request, "content-type");
	const char* parser_name = NULL;
    http_response_t* response = NULL;
    librdf_node *context = NULL;
    librdf_stream *stream = NULL;
    librdf_parser *parser = NULL;
    librdf_uri *base_uri = librdf_new_uri(world, (const unsigned char*)uri);
	unsigned char *buffer = NULL;

	if (!uri || strlen(uri)<1) {
		return redstore_error_page(REDSTORE_INFO, HTTP_BAD_REQUEST, "Invalid Graph URI.");
	}

	// FIXME: stream the input data, instead of storing it in memory first
	
	// Check we have a content_length header
	if (!content_length) {
		response = redstore_error_page(REDSTORE_INFO, HTTP_BAD_REQUEST, "Missing content length header.");
		goto CLEANUP;
	}
	
	// Allocate memory and read in the input data
    buffer = malloc(atoi(content_length));
	if (!buffer) {
		response = redstore_error_page(REDSTORE_ERROR, HTTP_INTERNAL_SERVER_ERROR, "Failed to allocate memory for input data.");
		goto CLEANUP;
	}
	
	// FIXME: do this better and check for errors
	fread(buffer, 1, atoi(content_length), request->socket);

	context = librdf_new_node_from_uri_string(world,(const unsigned char*)uri);
    if (!context) {
		response = redstore_error_page(REDSTORE_ERROR, HTTP_INTERNAL_SERVER_ERROR, "Failed to create graph node.");
		goto CLEANUP;
    }

	parser_name = librdf_parser_guess_name2(world, content_type, buffer, NULL);
    if (!parser_name) {
		response = redstore_error_page(REDSTORE_INFO, HTTP_INTERNAL_SERVER_ERROR, "Failed to guess parser type.");
		goto CLEANUP;
    }
	redstore_debug("Parsing using: %s", parser_name);
	
	parser = librdf_new_parser(world, parser_name, NULL, NULL);
    if (!parser) {
		response = redstore_error_page(REDSTORE_INFO, HTTP_INTERNAL_SERVER_ERROR, "Failed to create parser.");
		goto CLEANUP;
    }

	stream = librdf_parser_parse_string_as_stream(parser, buffer, base_uri);
    if (!stream) {
		response = redstore_error_page(REDSTORE_INFO, HTTP_INTERNAL_SERVER_ERROR, "Failed to parse data.");
		goto CLEANUP;
    }

	// FIXME: delete existing statements
	// FIXME: check for errors
	if (librdf_model_context_add_statements(model, context, stream)) {
		response = redstore_error_page(REDSTORE_ERROR, HTTP_INTERNAL_SERVER_ERROR, "Failed to add parsed statements to graph.");
		goto CLEANUP;
	}
	
	response = redstore_error_page(REDSTORE_DEBUG, HTTP_OK, "Successfully stored data in graph.");

CLEANUP:
    //librdf_free_stream(stream);
    if (context) librdf_free_node(context);
    if (content_length) free(content_length);
    if (content_type) free(content_type);
    if (buffer) free(buffer);
    if (stream) librdf_free_stream(stream);
    if (parser) librdf_free_parser(parser);

    return response;
}
http_response_t* handle_graph_delete(http_request_t *request, void* user_data)
{
    librdf_node *context = get_graph_context(request);
    http_response_t* response;

	// Create node
    if (!context) {
         return redstore_error_page(REDSTORE_INFO, HTTP_NOT_FOUND, "Graph not found.");
	}

    redstore_info("Deleting graph: %s", http_request_get_path_glob(request));
    
    if (librdf_model_context_remove_statements(model,context)) {
    	response = redstore_error_page(REDSTORE_ERROR, HTTP_INTERNAL_SERVER_ERROR, "Error while trying to delete graph");
    } else {
    	response = redstore_error_page(REDSTORE_INFO, HTTP_OK, "Successfully deleted Graph");
    }
    
    librdf_free_node(context);
    
    return response;
}

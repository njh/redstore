#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "redstore.h"


redhttp_response_t* handle_load_get(redhttp_request_t *request, void* user_data)
{
	redhttp_response_t* response = redhttp_response_new(REDHTTP_OK, NULL);
	page_append_html_header(response, "Load URI");
	redhttp_response_content_append(response,
        "<form method=\"post\" action=\"/load\"><div>\n"
        "<label for=\"uri\">URI:</label> <input id=\"uri\" name=\"uri\" type=\"text\" size=\"40\" /><br />\n"
        "<label for=\"base-uri\">Base URI:</label> <input id=\"base-uri\" name=\"base-uri\" type=\"text\" size=\"40\" /> <i>(optional)</i><br />\n"
        "<label for=\"graph\">Graph:</label> <input id=\"graph\" name=\"graph\" type=\"text\" size=\"40\" /> <i>(optional)</i><br />\n"
        "<input type=\"reset\" /> <input type=\"submit\" />\n"
        "</div></form>\n"
	);
	page_append_html_footer(response);
	return response;
}

redhttp_response_t* load_stream_into_graph(librdf_stream *stream, librdf_uri *graph_uri)
{
    redhttp_response_t* response = NULL;
    librdf_node *graph = NULL;
	size_t count = 0;

    graph = librdf_new_node_from_uri(world, graph_uri);
	if (!graph) {
		return redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR, "librdf_new_node_from_uri failed for Graph URI");
    }

	while(!librdf_stream_end(stream)) {
		librdf_statement *statement = librdf_stream_get_object(stream);
		if(!statement) {
    		response = redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR, "Failed to get statement from stream.");
			break;
		}
          
		if (librdf_model_context_add_statement(model, graph, statement)) {
    		response = redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR, "Failed to add statement to graph.");
			break;
		}
		count++;
		librdf_stream_next(stream);
	}
	
	// FIXME: check for parse errors or parse warnings
	if (!response) {
		response = redhttp_response_new(REDHTTP_OK, NULL);
		page_append_html_header(response, "Success");
		redstore_info("Added %d triples to graph.", count);
		redhttp_response_content_append(response, "<p>Added %d triples to graph: %s</p>", count, librdf_uri_as_string(graph_uri));
		page_append_html_footer(response);
		import_count++;
	}
	
	librdf_free_node(graph);
	
	return response;
}

redhttp_response_t* handle_load_post(redhttp_request_t *request, void* user_data)
{
	char* uri_arg = redhttp_request_get_argument(request, "uri");
	char* base_arg = redhttp_request_get_argument(request, "base-uri");
	char* graph_arg = redhttp_request_get_argument(request, "graph");
    librdf_uri *uri = NULL, *base_uri = NULL, *graph_uri = NULL;
    redhttp_response_t* response = NULL;
    librdf_parser *parser = NULL;
    librdf_stream *stream = NULL;

    if (!uri_arg) {
        response = redstore_error_page(REDSTORE_INFO, REDHTTP_BAD_REQUEST, "Missing URI to load");
        goto CLEANUP;
    }
    
    uri = librdf_new_uri(world, (const unsigned char *)uri_arg);
	if (!uri) {
        response = redstore_error_page(REDSTORE_ERROR, REDHTTP_BAD_REQUEST, "librdf_new_uri failed for URI");
        goto CLEANUP;
    }
    
    if (base_arg) {
    	base_uri = librdf_new_uri(world, (const unsigned char *)base_arg);
    } else {
    	base_uri = librdf_new_uri(world, (const unsigned char *)uri_arg);
    }
	if (!base_uri) {
        response = redstore_error_page(REDSTORE_ERROR, REDHTTP_BAD_REQUEST, "librdf_new_uri failed for Base URI");
        goto CLEANUP;
    }
    
    if (graph_arg) {
    	graph_uri = librdf_new_uri(world, (const unsigned char *)graph_arg);
    } else {
    	graph_uri = librdf_new_uri(world, (const unsigned char *)uri_arg);
    }
	if (!graph_uri) {
        response = redstore_error_page(REDSTORE_ERROR, REDHTTP_BAD_REQUEST, "librdf_new_uri failed for Graph URI");
        goto CLEANUP;
    }

    redstore_info("Loading URI: %s", librdf_uri_as_string(uri));
    redstore_debug("Base URI: %s", librdf_uri_as_string(base_uri));
    redstore_debug("Graph URI: %s", librdf_uri_as_string(graph_uri));

	// FIXME: allow user to choose the parser that is used
	parser = librdf_new_parser(world, "guess", NULL, NULL);
	if(!parser) {
		response = redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR, "Failed to create parser");
		goto CLEANUP;
	}
    
    stream = librdf_parser_parse_as_stream(parser, uri, base_uri);
    if(!stream) {
    	response = redstore_error_page(REDSTORE_ERROR, REDHTTP_INTERNAL_SERVER_ERROR, "Failed to parse RDF as stream.");
		goto CLEANUP;
	}
    
    
    response = load_stream_into_graph(stream, graph_uri);


CLEANUP:
	if (stream) librdf_free_stream(stream);
	if (parser) librdf_free_parser(parser);
	if (graph_uri) librdf_free_uri(graph_uri);
	if (base_uri) librdf_free_uri(base_uri);
	if (uri) librdf_free_uri(uri);
	if (base_arg) free(base_arg);
	if (graph_arg) free(graph_arg);
	if (uri_arg) free(uri_arg);

	return response;
}


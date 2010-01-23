#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "redstore.h"


http_response_t* redstore_error_page(int level, int code, const char* message)
{
	redstore_log(level, message);
	return http_response_new_error_page(code, message);
}

void page_append_html_header(http_response_t *response, const char* title)
{
	http_response_content_append(response,         
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
        "          \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
        "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
        "<head>\n"
        "  <title>RedStore - %s</title>\n"
        "</head>\n"
        "<body>\n"
        "<h1>%s</h1>\n", title, title
	);
}

void page_append_html_footer(http_response_t *response)
{
	http_response_content_append(response,         
        "<hr /><address>%s librdf/%s raptor/%s rasqal/%s</address>\n"
        "</body></html>\n",
        PACKAGE_NAME "/" PACKAGE_VERSION,
        librdf_version_string,
        raptor_version_string,
        rasqal_version_string
	);
}

http_response_t* handle_page_home(http_request_t *request, void* user_data)
{
	http_response_t* response = http_response_new(HTTP_OK, NULL);
	page_append_html_header(response, "RedStore");
	http_response_content_append(response,
        "<ul>\n"
        "  <li><a href=\"/query\">SPARQL Query Page</a></li>\n"
        "  <li><a href=\"/info\">Information</a></li>\n"
        "  <li><a href=\"/data\">Named Graphs</a></li>\n"
        "  <li><a href=\"/formats\">Supported Formats</a></li>\n"
        "</ul>\n"
	);
	page_append_html_footer(response);
	return response;
}

http_response_t* handle_page_formats(http_request_t *request, void* user_data)
{
	http_response_t* response;
	int i;
	
	response = http_response_new(HTTP_OK, NULL);
	page_append_html_header(response, "Supported Formats");
    
    http_response_content_append(response, "<h2>RDF Parsers</h2>\n");
    http_response_content_append(response, "<table border=\"1\">\n");
    http_response_content_append(response, "<tr><th>Name</th><th>Description</th></tr>\n");
    for(i=0; 1; i++) {
        const char *name, *label;
        if(librdf_parser_enumerate(world, i, &name, &label))
            break;
        http_response_content_append(response, "<tr><td>%s</td><td>%s</td></tr>\n", name, label);
    }
    http_response_content_append(response, "</table>\n");

    http_response_content_append(response, "<h2>RDF Serialisers</h2>\n");
    http_response_content_append(response, "<table border=\"1\">\n");
    http_response_content_append(response, "<tr><th>Name</th><th>Description</th><th>MIME Type</th><th>URI</th></tr>\n");
    for(i=0; serialiser_info[i].name; i++) {
        if(!serialiser_info[i].label) continue;
        http_response_content_append(response, "<tr><td>%s</td><td>%s</td><td>%s</td>",
                serialiser_info[i].name, serialiser_info[i].label, serialiser_info[i].mime_type);
        if (serialiser_info[i].uri) {
        	http_response_content_append(response, "<td><a href=\"%s\">More Info</a></td>\n", serialiser_info[i].uri);
        }
        http_response_content_append(response, "</tr>\n");
    }
    http_response_content_append(response, "</table>\n");

    http_response_content_append(response, "<h2>Query Result Formatters</h2>\n");
    http_response_content_append(response, "<table border=\"1\">\n");
    http_response_content_append(response, "<tr><th>Name</th><th>Description</th><th>MIME Type</th><th>URI</th></tr>\n");
    for(i=0; 1; i++) {
        const char *name, *label, *mime_type;
        const unsigned char *uri;
        if(librdf_query_results_formats_enumerate(world, i, &name, &label, &uri, &mime_type))
            break;
        http_response_content_append(response, "<tr><td>%s</td><td>%s</td><td>%s</td><td><a href=\"%s\">More Info</a></td></tr>\n", name, label, mime_type, uri);
    }
    http_response_content_append(response, "</table>\n");
	page_append_html_footer(response);

    return response;
}

static int context_count(librdf_storage *storage)
{
    librdf_iterator* iterator = NULL;
    int count = 0;
    
    assert(storage != NULL);

    iterator = librdf_storage_get_contexts(storage);
    if(!iterator) {
        redstore_error("librdf_storage_get_contexts returned NULL");
        return -1;
    }

    while(!librdf_iterator_end(iterator)) {
    	count++;
        librdf_iterator_next(iterator);
    }

    librdf_free_iterator(iterator);

	return count;
}


http_response_t* handle_page_info(http_request_t *request, void* user_data)
{
	http_response_t* response;

	response = http_response_new(HTTP_OK, NULL);
	page_append_html_header(response, "Store Information");
    http_response_content_append(response, "<dl>\n");
    http_response_content_append(response, "<dt>Storage Name</dt><dd>%s</dd>\n", storage_name);
    http_response_content_append(response, "<dt>Storage Type</dt><dd>%s</dd>\n", storage_type);
    http_response_content_append(response, "<dt>Triple Count</dt><dd>%d</dd>\n", librdf_storage_size(storage));
    http_response_content_append(response, "<dt>Named Graph Count</dt><dd>%d</dd>\n", context_count(storage));
    http_response_content_append(response, "<dt>HTTP Request Count</dt><dd>%d</dd>\n", request_count);
    http_response_content_append(response, "<dt>SPARQL Query Count</dt><dd>%d</dd>\n", query_count);
    http_response_content_append(response, "</dl>\n");
	page_append_html_footer(response);

    return response;
}

http_response_t* handle_page_query(http_request_t *request, void* user_data)
{
	http_response_t* response = http_response_new(HTTP_OK, NULL);
	page_append_html_header(response, "SPARQL Query");
    
    http_response_content_append(response, 
        "<form action=\"../sparql\" method=\"get\">\n"
        "<textarea name=\"query\" cols=\"80\" rows=\"18\">\n"
        "PREFIX rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>\n"
        "PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>\n"
        "\n"
        "SELECT * WHERE {\n"
        " ?s ?p ?o\n"
        "} LIMIT 10\n"
        "</textarea><br>\n"
        "Output Format: <select name=\"format\">\n"
        "  <option value=\"html\">HTML</option>\n"
        "  <option value=\"text\">Plain Text</option>\n"
        "  <option value=\"xml\">XML</option>\n"
        "  <option value=\"json\">JSON</option>\n"
        "</select>\n"
        "<input type=\"reset\"> "
        "<input type=\"submit\" value=\"Execute\">\n"
        "</form>\n"
    );
  
    // FIXME: list output formats based on enumeration of formats that Redland supports

	page_append_html_footer(response);

    return response;
}

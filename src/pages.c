#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "redstore.h"


redhttp_response_t* redstore_error_page(int level, int code, const char* message)
{
	redstore_log(level, message);
	return redhttp_response_new_error_page(code, message);
}

void page_append_html_header(redhttp_response_t *response, const char* title)
{
	redhttp_response_content_append(response,         
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

void page_append_html_footer(redhttp_response_t *response)
{
	redhttp_response_content_append(response,         
        "<hr /><address>%s librdf/%s raptor/%s rasqal/%s</address>\n"
        "</body></html>\n",
        PACKAGE_NAME "/" PACKAGE_VERSION,
        librdf_version_string,
        raptor_version_string,
        rasqal_version_string
	);
}

redhttp_response_t* handle_page_home(redhttp_request_t *request, void* user_data)
{
	redhttp_response_t* response = redhttp_response_new(REDHTTP_OK, NULL);
	page_append_html_header(response, "RedStore");
	redhttp_response_content_append(response,
        "<ul>\n"
        "  <li><a href=\"/query\">SPARQL Query Page</a></li>\n"
        "  <li><a href=\"/load\">Load URI</a></li>\n"
        "  <li><a href=\"/info\">Information</a></li>\n"
        "  <li><a href=\"/data\">Named Graphs</a></li>\n"
        "  <li><a href=\"/formats\">Supported Formats</a></li>\n"
        "</ul>\n"
	);
	page_append_html_footer(response);
	return response;
}

redhttp_response_t* handle_page_formats(redhttp_request_t *request, void* user_data)
{
	redhttp_response_t* response;
	int i;
	
	response = redhttp_response_new(REDHTTP_OK, NULL);
	page_append_html_header(response, "Supported Formats");
    
    redhttp_response_content_append(response, "<h2>RDF Parsers</h2>\n");
    redhttp_response_content_append(response, "<table border=\"1\">\n");
    redhttp_response_content_append(response, "<tr><th>Name</th><th>Description</th></tr>\n");
    for(i=0; 1; i++) {
        const char *name, *label;
        if(librdf_parser_enumerate(world, i, &name, &label))
            break;
        redhttp_response_content_append(response, "<tr><td>%s</td><td>%s</td></tr>\n", name, label);
    }
    redhttp_response_content_append(response, "</table>\n");

    redhttp_response_content_append(response, "<h2>RDF Serialisers</h2>\n");
    redhttp_response_content_append(response, "<table border=\"1\">\n");
    redhttp_response_content_append(response, "<tr><th>Name</th><th>Description</th><th>MIME Type</th><th>URI</th></tr>\n");
    for(i=0; serialiser_info[i].name; i++) {
        if(!serialiser_info[i].label) continue;
        redhttp_response_content_append(response, "<tr><td>%s</td><td>%s</td><td>%s</td>",
                serialiser_info[i].name, serialiser_info[i].label, serialiser_info[i].mime_type);
        if (serialiser_info[i].uri) {
        	redhttp_response_content_append(response, "<td><a href=\"%s\">More Info</a></td>\n", serialiser_info[i].uri);
        }
        redhttp_response_content_append(response, "</tr>\n");
    }
    redhttp_response_content_append(response, "</table>\n");

    redhttp_response_content_append(response, "<h2>Query Result Formatters</h2>\n");
    redhttp_response_content_append(response, "<table border=\"1\">\n");
    redhttp_response_content_append(response, "<tr><th>Name</th><th>Description</th><th>MIME Type</th><th>URI</th></tr>\n");
    for(i=0; 1; i++) {
        const char *name, *label, *mime_type;
        const unsigned char *uri;
        if(librdf_query_results_formats_enumerate(world, i, &name, &label, &uri, &mime_type))
            break;
        redhttp_response_content_append(response, "<tr><td>%s</td><td>%s</td><td>%s</td><td><a href=\"%s\">More Info</a></td></tr>\n", name, label, mime_type, uri);
    }
    redhttp_response_content_append(response, "</table>\n");
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


redhttp_response_t* handle_page_info(redhttp_request_t *request, void* user_data)
{
	redhttp_response_t* response;

	response = redhttp_response_new(REDHTTP_OK, NULL);
	page_append_html_header(response, "Store Information");
    redhttp_response_content_append(response, "<dl>\n");
    redhttp_response_content_append(response, "<dt>Storage Name</dt><dd>%s</dd>\n", storage_name);
    redhttp_response_content_append(response, "<dt>Storage Type</dt><dd>%s</dd>\n", storage_type);
    redhttp_response_content_append(response, "<dt>Triple Count</dt><dd>%d</dd>\n", librdf_storage_size(storage));
    redhttp_response_content_append(response, "<dt>Named Graph Count</dt><dd>%d</dd>\n", context_count(storage));
    redhttp_response_content_append(response, "<dt>HTTP Request Count</dt><dd>%lu</dd>\n", request_count);
    redhttp_response_content_append(response, "<dt>Successful Import Count</dt><dd>%lu</dd>\n", import_count);
    redhttp_response_content_append(response, "<dt>SPARQL Query Count</dt><dd>%lu</dd>\n", query_count);
    redhttp_response_content_append(response, "</dl>\n");
	page_append_html_footer(response);

    return response;
}

redhttp_response_t* handle_page_query(redhttp_request_t *request, void* user_data)
{
	redhttp_response_t* response = redhttp_response_new(REDHTTP_OK, NULL);
	page_append_html_header(response, "SPARQL Query");
    
    redhttp_response_content_append(response, 
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

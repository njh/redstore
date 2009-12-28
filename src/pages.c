
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <librdf.h>

#include "config.h"
#include "redstore.h"

#define MESSAGE_BUFFER_SIZE   (1024)

int handle_error(http_request_t *request, unsigned int status)
{
    char *message = (char*)malloc(MESSAGE_BUFFER_SIZE);
    const char *title;
    int ret = 0;
    
    if (status == MHD_HTTP_NOT_FOUND) {
        title = "Not Found";
        snprintf(message, MESSAGE_BUFFER_SIZE, "<p>The requested URL %s was not found on this server.</p>", request->url);
    } else {
        title = "Unknown Error";
        snprintf(message, MESSAGE_BUFFER_SIZE, "<p>An unknown error (%d) occurred.</p>", status);
    }
    
    ret = handle_html_page(request, status, title, message);
    free(message);
    
    return ret;
}


int handle_html_page(http_request_t *request, unsigned int status, 
                     const char* title, const char* content)
{
    FILE* stream = NULL;
    char *page_buffer = NULL;
    size_t page_size = 0;
    int ret = 0;

    stream = open_memstream(&page_buffer, &page_size);
    if(!stream) {
        redstore_error("Failed to open open_memstream");
        return handle_error(request, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
    
    fprintf(stream,
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
        "          \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
        "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
        "<head>\n"
        "  <title>RedStore - %s</title>\n"
        "</head>\n"
        "<body>\n"
        "<h1>%s</h1>\n"
        "%s\n"
        "<hr /><address>%s Redland/%s</address>\n"
        "</body></html>\n",
        title, title, content,
        PACKAGE_NAME "/" PACKAGE_VERSION,
        librdf_version_string
    );
    
    fclose(stream);

    ret = handle_static_data(request, status, (void*)page_buffer, page_size, "text/html");
           
    free(page_buffer);
    return ret;
}


int handle_static_data(http_request_t *request, unsigned int status,
                       void* content, size_t content_size,
                       const char* content_type)
{
    struct MHD_Response *response;
    int ret;
    
    response = MHD_create_response_from_data(
        content_size, content,
        MHD_NO, MHD_YES
    );

    MHD_add_response_header(response, "Content-Type", content_type);
    MHD_add_response_header(response, "Last-Modified", BUILD_TIME);
    MHD_add_response_header(response, "Server", PACKAGE_NAME "/" PACKAGE_VERSION);

    ret = MHD_queue_response(request->connection, status, response);
    MHD_destroy_response(response);
    
    return ret;
}


int handle_homepage(http_request_t *request)
{
    const char data[] = "<p>Hello World!</p>";

    return handle_html_page(request, MHD_HTTP_OK, "Homepage", data);
}

int handle_querypage(http_request_t *request)
{
    const char page[] = 
        "<form action=\"../sparql\" method=\"post\">\n"
        "<textarea name=\"query\" cols=\"80\" rows=\"18\">\n"
        "PREFIX rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>\n"
        "PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>\n"
        "\n"
        "SELECT * WHERE {\n"
        " ?s ?p ?o\n"
        "} LIMIT 10\n"
        "</textarea><br>\n"
        "<input type=\"reset\">\n"
        "<input type=\"submit\" value=\"Execute\">"
        "</form>\n";
    return handle_html_page(request, MHD_HTTP_OK, "SPARQL Query", page);
}

int handle_graph_index(http_request_t *request)
{
    librdf_iterator* iterator = NULL;
    FILE* stream = NULL;
    char *page_buffer = NULL;
    size_t page_size = 0;
    int ret;

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
        librdf_node* node = (librdf_node*)librdf_iterator_get_object(iterator);
        if(!node) {
            redstore_error("librdf_iterator_get_next returned NULL");
            break;
        }
        
        fprintf(stream, "<li>");
        librdf_node_print(node, stream);
        fprintf(stream, "</li>\n");
        
        librdf_iterator_next(iterator);
    }
    fprintf(stream, "</ul>\n");
    
    librdf_free_iterator(iterator);
    fclose(stream);
   
    ret = handle_html_page(request, MHD_HTTP_OK, "Named Graphs", page_buffer);
            
    free(page_buffer);
    return ret;
}


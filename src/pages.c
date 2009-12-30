
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <librdf.h>

#include "redstore.h"


/* NOTE: content will be freed after the response is sent */
http_response_t* new_http_response(http_request_t *request, unsigned int status,
                       void* content, size_t content_size,
                       const char* content_type)
{
    http_response_t* response = malloc(sizeof(response));
    if (!response) return NULL;
    
    response->mhd_response = MHD_create_response_from_data(
        content_size, content,
        MHD_YES, MHD_NO
    );
    
    response->status = status;

    // Add headers
    MHD_add_response_header(response->mhd_response, "Content-Type", content_type);
    //MHD_add_response_header(response->mhd_response, "Last-Modified", BUILD_TIME);
    MHD_add_response_header(response->mhd_response, "Server", PACKAGE_NAME "/" PACKAGE_VERSION);

    return response;
}


#define ERROR_MESSAGE_BUFFER_SIZE   (1024)

http_response_t* handle_error(http_request_t *request, unsigned int status)
{
    char *message = (char*)malloc(ERROR_MESSAGE_BUFFER_SIZE);
    char *title;
    
    if (status == MHD_HTTP_NOT_FOUND) {
        title = "Not Found";
        snprintf(message, ERROR_MESSAGE_BUFFER_SIZE, "<p>The requested URL %s was not found on this server.</p>", request->url);
    } else if (status == MHD_HTTP_METHOD_NOT_ALLOWED) {
        title = "Method Not Allowed";
        snprintf(message, ERROR_MESSAGE_BUFFER_SIZE, "<p>The requested method %s is not allowed for the URL %s.</p>", request->method, request->url);
    } else if (status == MHD_HTTP_INTERNAL_SERVER_ERROR) {
        title = "Internal Server Error";
        snprintf(message, ERROR_MESSAGE_BUFFER_SIZE, "<p>The server encountered an unexpected condition which prevented it from fulfilling the request.</p>");
    } else if (status == MHD_HTTP_NOT_IMPLEMENTED) {
        title = "Not Implemented";
        snprintf(message, ERROR_MESSAGE_BUFFER_SIZE, "<p>Sorry, this functionality has not been implemented.</p>");
    } else {
        title = "Unknown Error";
        snprintf(message, ERROR_MESSAGE_BUFFER_SIZE, "<p>An unknown error (%d) occurred.</p>", status);
    }

    return handle_html_page(request, status, title, message);
}

http_response_t* handle_redirect(http_request_t *request, char* url)
{
    char *message = (char*)malloc(ERROR_MESSAGE_BUFFER_SIZE);
    http_response_t* response;
    
    snprintf(message, ERROR_MESSAGE_BUFFER_SIZE, "<p>The document has moved <a href=\"%s\">here</a>.</p>", url);
    response = handle_html_page(request, MHD_HTTP_MOVED_PERMANENTLY, "Moved Permanently", message);
    MHD_add_response_header(response->mhd_response, MHD_HTTP_HEADER_LOCATION, url);
    free(url);

    return response;
}


/* NOTE: content will be freed after the response is sent */
http_response_t* handle_html_page(http_request_t *request, unsigned int status, 
                     const char* title, char* content)
{
    FILE* stream = NULL;
    char *page_buffer = NULL;
    size_t page_size = 0;

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
        "<hr /><address>%s librdf/%s libraptor/%s librasqal/%s</address>\n"
        "</body></html>\n",
        title, title, content,
        PACKAGE_NAME "/" PACKAGE_VERSION,
        librdf_version_string,
        raptor_version_string,
        rasqal_version_string
    );
    
    fclose(stream);
    free(content);

    return new_http_response(request, status, (void*)page_buffer, page_size, "text/html");
}


http_response_t* handle_homepage(http_request_t *request)
{
    const char page[] = 
        "<ul>\n"
        "<li><a href=\"/query\">SPARQL Query Page</a></li>"
        "<li><a href=\"/graphs\">Named Graphs</a></li>"
        "</ul>\n";

    return handle_html_page(request, MHD_HTTP_OK, "RedStore", strdup(page));
}

http_response_t* handle_querypage(http_request_t *request)
{
    const char page[] = 
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
        "</form>\n";
  
    // FIXME: list output formats based on enumeration of formats
    // that Redland supports

    return handle_html_page(request, MHD_HTTP_OK, "SPARQL Query", strdup(page));
}

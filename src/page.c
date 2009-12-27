#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <librdf.h>

#include "config.h"
#include "redstore.h"

#define MESSAGE_BUFFER_SIZE   (1024)
#define PAGE_BUFFER_SIZE      (2048)


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
    char *page_buffer = malloc(PAGE_BUFFER_SIZE);
    int ret = 0;
    
    snprintf(page_buffer, PAGE_BUFFER_SIZE,
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

    ret = handle_static_data(request, status,
           (void*)page_buffer, strlen(page_buffer), "text/html");
           
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

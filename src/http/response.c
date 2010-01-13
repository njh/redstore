#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <errno.h>
#include <sys/types.h>

#include "redhttpd.h"


http_response_t* http_response_new(int code, const char* message)
{
    http_response_t* response;
    
    assert(code>=100 && code<1000);

    response = calloc(1, sizeof(http_response_t));
    if (!response) {
        perror("failed to allocate memory for http_response_t");
        return NULL;
    } else {
        response->status_code = code;
        if (message == NULL) {
            switch(code) {
                case 200: message = "OK"; break;
                case 201: message = "Created"; break;
                case 202: message = "Accepted"; break;
                case 204: message = "No Content"; break;
                case 301: message = "Moved Permanently"; break;
                case 302: message = "Moved Temporarily"; break;
                case 304: message = "Not Modified"; break;
                case 400: message = "Bad Request"; break;
                case 401: message = "Unauthorized"; break;
                case 403: message = "Forbidden"; break;
                case 404: message = "Not Found"; break;
                case 500: message = "Internal Server Error"; break;
                case 501: message = "Not Implemented"; break;
                case 502: message = "Bad Gateway"; break;
                case 503: message = "Service Unavailable"; break;
                default: message = "Unknown"; break;
            }
        }
        response->status_message = strdup(message);
    }
        
    return response;
}

void http_response_content_append(http_response_t* response, const char* string)
{
    assert(response != NULL);
    assert(string != NULL);

    // Does the buffer already exist?
    if (!response->content_buffer) {
        response->content_buffer_size = BUFSIZ + strlen(string);
        response->content_buffer = malloc(response->content_buffer_size);
        response->content_length = 0;
    }

    // Is the buffer big enough?
    if (response->content_buffer_size - response->content_length < strlen(string)) {
        response->content_buffer_size += strlen(string) + BUFSIZ;
        response->content_buffer = realloc(response->content_buffer, response->content_buffer_size);
    }
    
    // Perform the append
    strcat(response->content_buffer + response->content_length, string);
    response->content_length += strlen(string);
}

void http_response_set_content(http_response_t* response, const char* data, size_t length, const char* type)
{
    assert(response != NULL);
    assert(data != NULL);
    assert(length != 0);
    assert(type != NULL);

    response->content_buffer = realloc(response->content_buffer, length);
    memcpy(response->content_buffer, data, length);
    http_headers_add(&response->headers, "Content-Type", type);
}

http_response_t* http_response_error_page(int code, const char* explanation)
{
    http_response_t* response = http_response_new(code, NULL);
    char *code_str = calloc(1, BUFSIZ);

    // FIXME: check for memory allocation error

    assert(code>=100 && code<1000);
    snprintf(code_str, BUFSIZ, "%d ", code);

    http_headers_add(&response->headers, "Content-Type", "text/html");
    http_response_content_append(response, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n");
    http_response_content_append(response, "<html><head><title>");
    http_response_content_append(response, code_str);
    http_response_content_append(response, response->status_message);
    http_response_content_append(response, "</title></head>\n");
    http_response_content_append(response, "<body><h1>");
    http_response_content_append(response, code_str);
    http_response_content_append(response, response->status_message);
    http_response_content_append(response, "</h1>\n");
    
    if (explanation) {
        http_response_content_append(response, "<p>");
        http_response_content_append(response, explanation);
        http_response_content_append(response, "</p>\n");
    }
    http_response_content_append(response, "</body></html>\n");
    
    free(code_str);
    
    return response;
}

void http_response_free(http_response_t* response)
{
    assert(response != NULL);

    if (response->status_message) free(response->status_message);
    if (response->content_buffer) free(response->content_buffer);

    http_headers_free(&response->headers);
}


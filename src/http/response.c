#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
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
                case 405: message = "Method Not Allowed"; break;
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

http_response_t* http_response_new_error_page(int code, const char* explanation)
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

http_response_t* http_response_new_redirect(const char* url)
{
    static const char MESSAGE_FMT[] = "<p>The document has moved <a href=\"%s\">here</a>.</p>";
    http_response_t* response;
    size_t message_length;
    char* message;

    message_length = snprintf(NULL, 0, MESSAGE_FMT, url);
    message = malloc(message_length+1);
    // FIXME: check for memory allocation error
    snprintf(message, message_length, MESSAGE_FMT, url);
    
    // Build the response
    response = http_response_new_error_page(301, message);
    http_response_add_header(response, "Location", url);
    free(message);

    return response;
}

void http_response_content_append(http_response_t* response, const char* string)
{
    assert(response != NULL);
    
    if (string == NULL || strlen(string) == 0)
    	return;

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


void http_response_add_header(http_response_t* response, const char* key, const char* value)
{
    http_headers_add(&response->headers, key, value);
}

void http_response_add_time_header(http_response_t* response, const char* key, time_t timer)
{
    static const char RFC1123FMT[] = "%a, %d %b %Y %H:%M:%S GMT";
    char *date_str = malloc(BUFSIZ);

    // FIXME: calculate the length of the date string instead of using BUFSIZ
    strftime(date_str, BUFSIZ, RFC1123FMT, gmtime(&timer));
    http_headers_add(&response->headers, key, date_str);
    
    free(date_str);
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

void http_response_free(http_response_t* response)
{
    assert(response != NULL);

    if (response->status_message) free(response->status_message);
    if (response->content_buffer) free(response->content_buffer);

    http_headers_free(&response->headers);
}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>

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
                case HTTP_OK:                    message = "OK"; break;
                case HTTP_CREATED:               message = "Created"; break;
                case HTTP_ACCEPTED:              message = "Accepted"; break;
                case HTTP_NO_CONTENT:            message = "No Content"; break;
                case HTTP_MOVED_PERMANENTLY:     message = "Moved Permanently"; break;
                case HTTP_MOVED_TEMPORARILY:     message = "Moved Temporarily"; break;
                case HTTP_SEE_OTHER:             message = "See Other"; break;
                case HTTP_NOT_MODIFIED:          message = "Not Modified"; break;
                case HTTP_BAD_REQUEST:           message = "Bad Request"; break;
                case HTTP_UNAUTHORIZED:          message = "Unauthorized"; break;
                case HTTP_FORBIDDEN:             message = "Forbidden"; break;
                case HTTP_NOT_FOUND:             message = "Not Found"; break;
                case HTTP_METHOD_NOT_ALLOWED:    message = "Method Not Allowed"; break;
                case HTTP_INTERNAL_SERVER_ERROR: message = "Internal Server Error"; break;
                case HTTP_NOT_IMPLEMENTED:       message = "Not Implemented"; break;
                case HTTP_BAD_GATEWAY:           message = "Bad Gateway"; break;
                case HTTP_SERVICE_UNAVAILABLE:   message = "Service Unavailable"; break;
                default:                         message = "Unknown"; break;
            }
        }
        response->status_message = strdup(message);
    }
        
    return response;
}

http_response_t* http_response_new_error_page(int code, const char* explanation)
{
    http_response_t* response = http_response_new(code, NULL);

    assert(code>=100 && code<1000);
    if (response == NULL) return NULL;

    http_headers_add(&response->headers, "Content-Type", "text/html");
    http_response_content_append(response, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n");
    http_response_content_append(response, "<html>\n");
    http_response_content_append(response, "<head><title>%d %s</title></head>\n", code, response->status_message);
    http_response_content_append(response, "<body><h1>%d %s</h1>\n", code, response->status_message);
    
    if (explanation) {
        http_response_content_append(response, "<p>%s</p>\n", explanation);
    }
    http_response_content_append(response, "</body></html>\n");

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
    response = http_response_new_error_page(HTTP_MOVED_PERMANENTLY, message);
    http_response_add_header(response, "Location", url);
    free(message);

    return response;
}

http_response_t* http_response_new_with_content(const char* data, size_t length, const char* type)
{
    http_response_t* response = http_response_new(HTTP_OK, NULL);
    http_response_set_content(response, data, length, type);
    return response;
}

void http_response_content_append(http_response_t* response, const char* fmt, ...)
{
    va_list args;
    size_t len;

    assert(response != NULL);
    
    if (fmt == NULL || strlen(fmt) == 0)
    	  return;
    
    // Get the length of the formatted string
    va_start(args, fmt);
    len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    // Does the buffer already exist?
    if (!response->content_buffer) {
        response->content_buffer_size = BUFSIZ + len;
        response->content_buffer = malloc(response->content_buffer_size);
        response->content_length = 0;
        // FIXME: check for memory allocation error
    }

    // Is the buffer big enough?
    if (response->content_buffer_size - response->content_length < len) {
        response->content_buffer_size += len + BUFSIZ;
        response->content_buffer = realloc(response->content_buffer, response->content_buffer_size);
        // FIXME: check for memory allocation error
    }
    
    // Add formatted string the buffer
    va_start(args, fmt);
    response->content_length += vsnprintf(
        &response->content_buffer[response->content_length],
        response->content_buffer_size - response->content_length - 1, fmt, args
    );
    va_end(args);
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
    assert(length > 0);
    assert(type != NULL);

    response->content_buffer = realloc(response->content_buffer, length);
    memcpy(response->content_buffer, data, length);
    response->content_length = length;
    http_headers_add(&response->headers, "Content-Type", type);
}

void http_response_free(http_response_t* response)
{
    assert(response != NULL);

    if (response->status_message) free(response->status_message);
    if (response->content_buffer) free(response->content_buffer);

    http_headers_free(&response->headers);
}


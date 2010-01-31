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

#include "redhttp.h"


redhttp_response_t* redhttp_response_new(int code, const char* message)
{
    redhttp_response_t* response;
    
    assert(code>=100 && code<1000);

    response = calloc(1, sizeof(redhttp_response_t));
    if (!response) {
        perror("failed to allocate memory for redhttp_response_t");
        return NULL;
    } else {
        response->status_code = code;
        if (message == NULL) {
            switch(code) {
                case REDHTTP_OK:                    message = "OK"; break;
                case REDHTTP_CREATED:               message = "Created"; break;
                case REDHTTP_ACCEPTED:              message = "Accepted"; break;
                case REDHTTP_NO_CONTENT:            message = "No Content"; break;
                case REDHTTP_MOVED_PERMANENTLY:     message = "Moved Permanently"; break;
                case REDHTTP_MOVED_TEMPORARILY:     message = "Moved Temporarily"; break;
                case REDHTTP_SEE_OTHER:             message = "See Other"; break;
                case REDHTTP_NOT_MODIFIED:          message = "Not Modified"; break;
                case REDHTTP_BAD_REQUEST:           message = "Bad Request"; break;
                case REDHTTP_UNAUTHORIZED:          message = "Unauthorized"; break;
                case REDHTTP_FORBIDDEN:             message = "Forbidden"; break;
                case REDHTTP_NOT_FOUND:             message = "Not Found"; break;
                case REDHTTP_METHOD_NOT_ALLOWED:    message = "Method Not Allowed"; break;
                case REDHTTP_INTERNAL_SERVER_ERROR: message = "Internal Server Error"; break;
                case REDHTTP_NOT_IMPLEMENTED:       message = "Not Implemented"; break;
                case REDHTTP_BAD_GATEWAY:           message = "Bad Gateway"; break;
                case REDHTTP_SERVICE_UNAVAILABLE:   message = "Service Unavailable"; break;
                default:                         message = "Unknown"; break;
            }
        }
        response->status_message = strdup(message);
    }
        
    return response;
}

redhttp_response_t* redhttp_response_new_error_page(int code, const char* explanation)
{
    redhttp_response_t* response = redhttp_response_new(code, NULL);

    assert(code>=100 && code<1000);
    if (response == NULL) return NULL;

    redhttp_headers_add(&response->headers, "Content-Type", "text/html");
    redhttp_response_content_append(response, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n");
    redhttp_response_content_append(response, "<html>\n");
    redhttp_response_content_append(response, "<head><title>%d %s</title></head>\n", code, response->status_message);
    redhttp_response_content_append(response, "<body><h1>%d %s</h1>\n", code, response->status_message);
    
    if (explanation) {
        redhttp_response_content_append(response, "<p>%s</p>\n", explanation);
    }
    redhttp_response_content_append(response, "</body></html>\n");

    return response;
}

redhttp_response_t* redhttp_response_new_redirect(const char* url)
{
    static const char MESSAGE_FMT[] = "<p>The document has moved <a href=\"%s\">here</a>.</p>";
    redhttp_response_t* response;
    size_t message_length;
    char* message;

    message_length = snprintf(NULL, 0, MESSAGE_FMT, url);
    message = malloc(message_length+1);
    // FIXME: check for memory allocation error
    snprintf(message, message_length, MESSAGE_FMT, url);
    
    // Build the response
    response = redhttp_response_new_error_page(REDHTTP_MOVED_PERMANENTLY, message);
    redhttp_response_add_header(response, "Location", url);
    free(message);

    return response;
}

redhttp_response_t* redhttp_response_new_with_content(const char* data, size_t length, const char* type)
{
    redhttp_response_t* response = redhttp_response_new(REDHTTP_OK, NULL);
    redhttp_response_set_content(response, data, length, type);
    return response;
}

void redhttp_response_content_append(redhttp_response_t* response, const char* fmt, ...)
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


void redhttp_response_add_header(redhttp_response_t* response, const char* key, const char* value)
{
    redhttp_headers_add(&response->headers, key, value);
}

void redhttp_response_add_time_header(redhttp_response_t* response, const char* key, time_t timer)
{
    static const char RFC1123FMT[] = "%a, %d %b %Y %H:%M:%S GMT";
    char *date_str = malloc(BUFSIZ);
    struct tm time_tm;

    // FIXME: calculate the length of the date string instead of using BUFSIZ
    gmtime_r(&timer, &time_tm);
    strftime(date_str, BUFSIZ, RFC1123FMT, &time_tm);
    redhttp_headers_add(&response->headers, key, date_str);
    
    free(date_str);
}


void redhttp_response_set_content(redhttp_response_t* response, const char* data, size_t length, const char* type)
{
    assert(response != NULL);
    assert(data != NULL);
    assert(length > 0);
    assert(type != NULL);

    response->content_buffer = realloc(response->content_buffer, length);
    memcpy(response->content_buffer, data, length);
    response->content_length = length;
    redhttp_headers_add(&response->headers, "Content-Type", type);
}

void redhttp_response_free(redhttp_response_t* response)
{
    assert(response != NULL);

    if (response->status_message) free(response->status_message);
    if (response->content_buffer) free(response->content_buffer);

    redhttp_headers_free(&response->headers);
    free(response);
}


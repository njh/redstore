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


http_request_t* http_request_new(void)
{
    http_request_t* request = calloc(1, sizeof(http_request_t));
    if (!request) {
        perror("failed to allocate memory for http_request_t");
        return NULL;
    }
        
    return request;
}


char* http_request_read_line(http_request_t *request)
{
    char *buffer = calloc(1, BUFSIZ);
    int buffer_size = BUFSIZ;
    int buffer_count = 0;

    assert(request != NULL);

    // FIXME: check memory was allocated
    
    buffer[0] = '\0';

    while (1) {
        // FIXME: is fgetc really slow way of doing things?
        int c = fgetc(request->socket);
        if (c<=0) {
            free(buffer);
            return NULL;
        }

        if (c == '\r') {
            buffer[buffer_count] = '\0';
        } else if (c == '\n') {
            buffer[buffer_count] = '\0';
            break;
        } else {
            buffer[buffer_count] = c;
        }
        
        buffer_count++;
        
        // Expand buffer ?
        if (buffer_count > (buffer_size - 1)) {
            buffer_size *= 2;
            buffer = realloc(buffer, buffer_size);
            // FIXME: check memory was allocated
        }
    }

    return buffer;
}

char* http_request_get_header(http_request_t *request, const char* key)
{
    return http_headers_get(&request->headers, key);
}

char* http_request_get_argument(http_request_t *request, const char* key)
{
    return http_headers_get(&request->arguments, key);
}

static void http_request_parse_arguments(http_request_t *request, const char *input)
{
    char *args, *ptr, *key, *value;

    assert(request != NULL);
    if (input == NULL) return;
    args = strdup(input);
    
    for(ptr = args; ptr && *ptr;)
    {
        key = ptr;
        ptr = strstr(key, "=");
        if (ptr == NULL)
            return;
        *ptr++ = '\0';
        
        value = ptr;
        ptr = strstr(value, "&");
        if (ptr != NULL)
        {
            *ptr++ = '\0';
        }
        
        key = http_url_unescape(key);
        value = http_url_unescape(value);
        http_headers_add(&request->arguments, key, value);
        free(key);
        free(value);
    }
    
    free(args);
}

int http_request_read_status_line(http_request_t *request)
{
    char *line, *ptr;
    char *method = NULL;
    char *url = NULL;
    char *version = NULL;
    
    assert(request != NULL);
    
    line = http_request_read_line(request);
    if (line == NULL || strlen(line) == 0) {
        // FAIL!
        return 400;
    }

    // Skip whitespace at the start
    for (ptr = line; isspace(*ptr); ptr++)
        continue;
        
    // Find the end of the method
    method = ptr;
    while (isalpha(*ptr))
        ptr++;
    *ptr++ = '\0';

    // Find the start of the url
    while (isspace(*ptr) && *ptr != '\n')
        ptr++;
    if (*ptr == '\n' || *ptr == '\0') {
        free(line);
        return 400;
    }
    url = ptr;

    // Find the end of the url
    ptr = &url[strlen(url)];
    while (isspace(*ptr))
        ptr--;
    *ptr = '\0';
    while (!isspace(*ptr) && ptr > url)
        ptr--;
    
    // Is there a version string at the end?
    if (ptr > url && strncasecmp("HTTP/",&ptr[1],5)==0) {
        version = &ptr[6];
        while (isspace(*ptr) && ptr > url)
            ptr--;
        ptr[1] = '\0';
    } else {
        version = "0.9";
    }
    
    // Is the URL valid?
    if (strlen(url)==0) {
        free(line);
        return 400;
    }

    request->method = strdup(method);
    request->url = strdup(url);
    request->version = strdup(version);
    
    // Separate the path from the query string
    for(ptr = url; *ptr && *ptr != '?'; ptr++);
    if (*ptr == '?') {
        *ptr = '\0';
        request->query_string = strdup(&ptr[1]);
    }
    request->path = http_url_unescape(url);
    
    http_request_parse_arguments(request, request->query_string);

    free(line);

    // Success
    return 0;
}

void http_request_send_response(http_request_t *request, http_response_t *response)
{
    static const char RFC1123FMT[] = "%a, %d %b %Y %H:%M:%S GMT";
    time_t timer = time(NULL);
    char date_str[80];
    
    assert(request != NULL);
    assert(response != NULL);
    
    if (!request->response_sent) {
        strftime(date_str, sizeof(date_str), RFC1123FMT, gmtime(&timer));
        http_headers_add(&response->headers, "Date", date_str);
        http_headers_add(&response->headers, "Connection", "Close");
      
        if (response->content_length) {
            // FIXME: must be better way to do int to string
            char *length_str = malloc(BUFSIZ);
            snprintf(length_str, BUFSIZ, "%d", (int)response->content_length);
            http_headers_add(&response->headers, "Content-Length", length_str);
            free(length_str);
        }
    
        if (strncmp(request->version, "0.9", 3)) {
            fprintf(request->socket, "HTTP/1.0 %d %s\r\n", response->status_code, response->status_message);
            http_headers_send(&response->headers, request->socket);
            fputs("\r\n", request->socket);
        }
        
        if (response->content_buffer) {
            fputs(response->content_buffer, request->socket);
        }
        
        request->response_sent = 1;
    }
}

void http_request_free(http_request_t* request)
{
    assert(request != NULL);

    if (request->method) free(request->method);
    if (request->url) free(request->url);
    if (request->version) free(request->version);
    if (request->path) free(request->path);
    if (request->query_string) free(request->query_string);
    
    if (request->socket) fclose(request->socket);

    http_headers_free(&request->headers);
}



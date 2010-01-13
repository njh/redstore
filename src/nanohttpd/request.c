/* Nano HTTP Server */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <errno.h>
#include <sys/types.h>

#include "nanohttpd.h"


http_request_t* http_request_new()
{
    http_request_t* request = calloc(1, sizeof(http_request_t));
    if (!request) {
        perror("failed to allocate memory for http_request_t");
        return NULL;
    }
        
    return request;
}


char* http_request_get_line(http_request_t *request)
{
    char *buffer = calloc(1, BUFSIZ);
    int buffer_size = BUFSIZ;
    int buffer_count = 0;
    int c;

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


static int http_read_request_line(http_request_t *request)
{
    char *line, *ptr;
    char *method = NULL;
    char *url = NULL;
    char *version = NULL;
    
    line = http_request_get_line(request);
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

    free(line);

    // Success
    return 0;
}

int http_request_send_response(http_request_t *request, http_response_t *response)
{
  static const char RFC1123FMT[] = "%a, %d %b %Y %H:%M:%S GMT";
  time_t timer = time(NULL);
  char date_str[80];

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


int http_request_process(FILE* file /*, client address */)
{
    http_request_t* request = http_request_new();
    http_response_t* response = NULL;

    if (!request) return -1;
    request->socket = file;
    // FIXME: store client address
    
    if (http_read_request_line(request)) {
        // FIXME: return with 400
        response = http_response_error_page(400, NULL);
    } else {
        response = http_response_error_page(200, "Everything is OK");
        printf("method: %s\n", request->method);
        printf("url: %s\n", request->url);
        printf("version: %s\n", request->version);

        printf("%s: %s\n", request->method, request->url);
    
        if (strncmp(request->version, "0.9", 3)) {
            // Read in the headers
            while(!feof(request->socket)) {
                char* line = http_request_get_line(request);
                if (line == NULL || strlen(line)==0) break;
                http_headers_parse_line(&request->headers, line);
                free(line);
            }
        }
    }

    // Send response
    http_request_send_response(request, response);

    http_request_free(request);

    // Success
    return 0;
}

void http_request_free(http_request_t* request)
{
    if (request->method) free(request->method);
    if (request->url) free(request->url);
    if (request->version) free(request->version);
    
    if (request->socket) fclose(request->socket);

    http_headers_free(&request->headers);
}


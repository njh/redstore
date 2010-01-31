#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#include <errno.h>
#include <sys/types.h>

#include "redhttp.h"


redhttp_request_t* redhttp_request_new(void)
{
    redhttp_request_t* request = calloc(1, sizeof(redhttp_request_t));
    if (!request) {
        perror("failed to allocate memory for redhttp_request_t");
        return NULL;
    }
        
    return request;
}


char* redhttp_request_read_line(redhttp_request_t *request)
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

char* redhttp_request_get_header(redhttp_request_t *request, const char* key)
{
    return redhttp_headers_get(&request->headers, key);
}

char* redhttp_request_get_argument(redhttp_request_t *request, const char* key)
{
    return redhttp_headers_get(&request->arguments, key);
}

void redhttp_request_set_path_glob(redhttp_request_t *request, const char* path_glob)
{
	// Free the old glob
	if (request->path_glob) {
		free(request->path_glob);
	}
	
	// Store the new glob
	if (path_glob && strlen(path_glob)) {
		request->path_glob = strdup(path_glob);
	} else {
		request->path_glob = NULL;
	}
}

const char* redhttp_request_get_path_glob(redhttp_request_t *request)
{
	return request->path_glob;
}

void redhttp_request_parse_arguments(redhttp_request_t *request, const char *input)
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
        
        key = redhttp_url_unescape(key);
        value = redhttp_url_unescape(value);
        redhttp_headers_add(&request->arguments, key, value);
        free(key);
        free(value);
    }
    
    free(args);
}


FILE* redhttp_request_get_socket(redhttp_request_t *request)
{
    return request->socket;
}


int redhttp_request_read_status_line(redhttp_request_t *request)
{
    char *line, *ptr;
    char *method = NULL;
    char *url = NULL;
    char *version = NULL;
    
    assert(request != NULL);
    
    line = redhttp_request_read_line(request);
    if (line == NULL || strlen(line) == 0) {
        // FAIL!
        if (line) free(line);
        return REDHTTP_BAD_REQUEST;
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
        return REDHTTP_BAD_REQUEST;
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
        return REDHTTP_BAD_REQUEST;
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
    request->path = redhttp_url_unescape(url);
    
    redhttp_request_parse_arguments(request, request->query_string);

    free(line);

    // Success
    return 0;
}

void redhttp_request_send_response(redhttp_request_t *request, redhttp_response_t *response)
{
    assert(request != NULL);
    assert(response != NULL);
    
    if (!response->headers_sent) {
    	const char *signature = redhttp_server_get_signature(request->server);
    
        redhttp_response_add_time_header(response, "Date", time(NULL));
        redhttp_response_add_header(response, "Connection", "Close");
        
        if (signature)
        	redhttp_response_add_header(response, "Server", signature);
      
        if (response->content_length) {
            // FIXME: must be better way to do int to string
            char *length_str = malloc(BUFSIZ);
            snprintf(length_str, BUFSIZ, "%d", (int)response->content_length);
            redhttp_response_add_header(response, "Content-Length", length_str);
            free(length_str);
        }
    
        if (strncmp(request->version, "0.9", 3)) {
            fprintf(request->socket, "HTTP/1.0 %d %s\r\n", response->status_code, response->status_message);
            redhttp_headers_send(&response->headers, request->socket);
            fputs("\r\n", request->socket);
        }
        
        response->headers_sent = 1;
    }

	if (response->content_buffer) {
		assert(response->content_length > 0);
		fwrite(response->content_buffer, 1, response->content_length, request->socket);
		// FIXME: check for error?
	}
}

void redhttp_request_free(redhttp_request_t* request)
{
    assert(request != NULL);

    if (request->method) free(request->method);
    if (request->url) free(request->url);
    if (request->version) free(request->version);
    if (request->path) free(request->path);
    if (request->path_glob) free(request->path_glob);
    if (request->query_string) free(request->query_string);
    
    if (request->socket) fclose(request->socket);

    redhttp_headers_free(&request->headers);
    redhttp_headers_free(&request->arguments);
    
    free(request);
}



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

const char* redhttp_request_get_header(redhttp_request_t *request, const char* key)
{
    return redhttp_headers_get(&request->headers, key);
}

void redhttp_request_add_header(redhttp_request_t *request, const char* key, const char* value)
{
    redhttp_headers_add(&request->headers, key, value);
}

const char* redhttp_request_get_argument(redhttp_request_t *request, const char* key)
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
		request->path_glob = calloc(1, strlen(path_glob)+1);
		strcpy(request->path_glob, path_glob);
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
    args = calloc(1, strlen(input)+1);
    strcpy(args, input);
    
    for(ptr = args; ptr && *ptr;)
    {
        key = ptr;
        ptr = strstr(key, "=");
        if (ptr == NULL)
            break;
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

void redhttp_request_set_method(redhttp_request_t *request, const char* method)
{
    assert(request != NULL);

    if (request->method)
        free(request->method);
        
    if (method) {
	int i, len = strlen(method);
        request->method = calloc(1,len+1);
	for(i=0; i<len; i++) {
            request->method[i] = toupper(method[i]);
	}
        request->method[i] = '\0';
    } else {
        request->method = NULL;
    }
}

const char* redhttp_request_get_method(redhttp_request_t *request)
{
    return request->method;
}

void redhttp_request_set_url(redhttp_request_t *request, const char* url)
{
    assert(request != NULL);

    // FIXME: repeated code
    if (request->url)
        free(request->url);
        
    if (url) {
        request->url = calloc(1, strlen(url)+1);
        strcpy(request->url, url);
    } else {
        request->url = NULL;
    }
}

const char* redhttp_request_get_url(redhttp_request_t *request)
{
    return request->url;
}

void redhttp_request_set_path(redhttp_request_t *request, const char* path)
{
    assert(request != NULL);

    // FIXME: repeated code
    if (request->path)
        free(request->path);
        
    if (path) {
        request->path = calloc(1, strlen(path)+1);
        strcpy(request->path, path);
    } else {
        request->path = NULL;
    }
}

const char* redhttp_request_get_path(redhttp_request_t *request)
{
    return request->path;
}

void redhttp_request_set_version(redhttp_request_t *request, const char* version)
{
    assert(request != NULL);

    // FIXME: repeated code
    if (request->version)
        free(request->version);
        
    if (version) {
        request->version = calloc(1, strlen(version)+1);
        strcpy(request->version, version);
    } else {
        request->version = NULL;
    }
}

const char* redhttp_request_get_version(redhttp_request_t *request)
{
    return request->version;
}

void redhttp_request_set_query_string(redhttp_request_t *request, const char* query_string)
{
    assert(request != NULL);

    // FIXME: repeated code
    if (request->query_string)
        free(request->query_string);
        
    if (query_string) {
        request->query_string = calloc(1, strlen(query_string)+1);
        strcpy(request->query_string, query_string);
    } else {
        request->query_string = NULL;
    }
}

const char* redhttp_request_get_query_string(redhttp_request_t *request)
{
    return request->query_string;
}

void redhttp_request_set_socket(redhttp_request_t *request, FILE* socket)
{
    request->socket = socket;
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
    while ((*ptr == '\0' || isspace(*ptr)) && ptr > url)
        ptr--;
    ptr[1] = '\0';
    while (!isspace(*ptr) && ptr > url)
        ptr--;
    
    // Is there a version string at the end?
    if (ptr > url && (strncmp("HTTP/",&ptr[1],5)==0 || strncmp("http/",&ptr[1],5)==0)) {
        version = &ptr[6];
        while (isspace(*ptr) && ptr > url)
            ptr--;
        ptr[1] = '\0';
    } else {
        version = "0.9";
    }

    redhttp_request_set_method(request, method);
    redhttp_request_set_url(request, url);
    redhttp_request_set_version(request, version);
    
    // Separate the path from the query string
    for(ptr = url; *ptr && *ptr != '?'; ptr++);
    if (*ptr == '?') {
        *ptr = '\0';
        redhttp_request_set_query_string(request, &ptr[1]);
    }
    request->path = redhttp_url_unescape(url);
    
    redhttp_request_parse_arguments(request, request->query_string);

    free(line);

    // Success
    return 0;
}


int redhttp_request_read(redhttp_request_t *request)
{
    int result = redhttp_request_read_status_line(request);
    if (result) return result;
    
    if (strncmp(request->version, "0.9", 3)) {
        // Read in the headers
        while(!feof(request->socket)) {
            char* line = redhttp_request_read_line(request);
            if (line == NULL || strlen(line)<1) {
                if (line) free(line);
                break;
            }
            redhttp_headers_parse_line(&request->headers, line);
            free(line);
        }
        
        // Read in PUT/POST content
        if (strncmp(request->method, "POST", 4)==0) {
            const char *content_type = redhttp_headers_get(&request->headers, "Content-Type");
            const char *content_length = redhttp_headers_get(&request->headers, "Content-Length");
            int bytes_read = 0;
    
            if (content_type==NULL || content_length==NULL) {
                return REDHTTP_BAD_REQUEST;
            } else if (strncmp(content_type, "application/x-www-form-urlencoded", 33)==0) {
                request->content_length = atoi(content_length);
                // FIXME: set maximum POST size
                request->content_buffer = calloc(1,request->content_length+1);
                // FIXME: check memory allocation succeeded
                
                bytes_read = fread(request->content_buffer, 1, request->content_length, request->socket);
                if (bytes_read != request->content_length) {
                    // FIXME: better response?
                    return REDHTTP_BAD_REQUEST;
                } else {
                    redhttp_request_parse_arguments(request, request->content_buffer);
                }
            }
        }
    }
    
    // Success
    return 0;
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



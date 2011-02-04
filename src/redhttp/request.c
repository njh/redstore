/*
    RedHTTP - a lightweight HTTP server library
    Copyright (C) 2010-2011 Nicholas J Humfrey <njh@aelius.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#include <errno.h>
#include <sys/types.h>

#include "redhttp_private.h"
#include "redhttp.h"


redhttp_request_t *redhttp_request_new(void)
{
  redhttp_request_t *request = calloc(1, sizeof(redhttp_request_t));
  if (!request) {
    perror("failed to allocate memory for redhttp_request_t");
    return NULL;
  }

  return request;
}

redhttp_request_t *redhttp_request_new_with_args(const char *method,
                                                 const char *url, const char *version)
{
  redhttp_request_t *request = redhttp_request_new();
  if (!request)
    return NULL;

  redhttp_request_set_method(request, method);
  redhttp_request_set_url(request, url);
  redhttp_request_set_version(request, version);
  return request;
}

char *redhttp_request_read_line(redhttp_request_t * request)
{
  char *buffer = calloc(1, BUFSIZ);
  int buffer_size = BUFSIZ;
  int buffer_count = 0;

  assert(request != NULL);
  if (!buffer)
    return NULL;

  // FIXME: check memory was allocated

  buffer[0] = '\0';

  while (1) {
    // FIXME: is fgetc really slow way of doing things?
    int c = fgetc(request->socket);
    if (c <= 0) {
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
      char *new_buf = realloc(buffer, buffer_size * 2);
      if (new_buf) {
        buffer = new_buf;
        buffer_size *= 2;
      } else {
        free(buffer);
        return NULL;
      }
    }
  }

  return buffer;
}

int redhttp_request_count_headers(redhttp_request_t * request)
{
  return redhttp_headers_count(&request->headers);
}

void redhttp_request_print_headers(redhttp_request_t * request, FILE * socket)
{
  redhttp_headers_print(&request->headers, socket);
}

const char *redhttp_request_get_header(redhttp_request_t * request, const char *key)
{
  return redhttp_headers_get(&request->headers, key);
}

void redhttp_request_add_header(redhttp_request_t * request, const char *key, const char *value)
{
  redhttp_headers_add(&request->headers, key, value);
}

int redhttp_request_count_arguments(redhttp_request_t * request)
{
  return redhttp_headers_count(&request->arguments);
}

void redhttp_request_print_arguments(redhttp_request_t * request, FILE * socket)
{
  redhttp_headers_print(&request->arguments, socket);
}

const char *redhttp_request_get_argument(redhttp_request_t * request, const char *key)
{
  return redhttp_headers_get(&request->arguments, key);
}

void redhttp_request_set_path_glob(redhttp_request_t * request, const char *path_glob)
{
  // Free the old glob
  if (request->path_glob)
    free(request->path_glob);

  // Store the new glob
  if (path_glob && strlen(path_glob)) {
    request->path_glob = redhttp_strdup(path_glob);
  }
}

const char *redhttp_request_get_path_glob(redhttp_request_t * request)
{
  return request->path_glob;
}

void redhttp_request_parse_arguments(redhttp_request_t * request, const char *input)
{
  char *args, *ptr, *key, *value;

  assert(request != NULL);
  if (!input)
    return;

  args = redhttp_strdup(input);
  if (!args)
    return;

  for (ptr = args; ptr && *ptr;) {
    key = ptr;
    ptr = strchr(key, '=');
    if (ptr == NULL)
      break;
    *ptr++ = '\0';

    value = ptr;
    ptr = strchr(value, '&');
    if (ptr != NULL) {
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


void redhttp_request_set_method(redhttp_request_t * request, const char *method)
{
  assert(request != NULL);

  if (request->method)
    free(request->method);

  if (method) {
    int i, len = strlen(method);
    request->method = calloc(1, len + 1);
    if (request->method) {
      for (i = 0; i < len; i++) {
        request->method[i] = toupper(method[i]);
      }
      request->method[i] = '\0';
    }
  } else {
    request->method = NULL;
  }
}

const char *redhttp_request_get_method(redhttp_request_t * request)
{
  return request->method;
}

void redhttp_request_set_url(redhttp_request_t * request, const char *url)
{
  assert(request != NULL);

  if (request->url)
    free(request->url);

  if (url) {
    char *ptr = NULL;
    char *path = NULL;
    size_t path_len = 0;

    // Store a copy of the URL
    request->url = redhttp_strdup(url);
    if (!request->url)
      return;

    // Check for query string
    ptr = strchr(url, '?');
    if (ptr) {
      path_len = (ptr - url);
      redhttp_request_set_query_string(request, &ptr[1]);
      redhttp_request_parse_arguments(request, &ptr[1]);
    } else {
      path_len = strlen(url);
    }

    // Unescape the path
    path = calloc(1, path_len + 1);
    if (path) {
      strncpy(path, url, path_len);
      path[path_len] = '\0';
      request->path = redhttp_url_unescape(path);
      free(path);
    }
  } else {
    request->url = NULL;
  }
}

const char *redhttp_request_get_url(redhttp_request_t * request)
{
  return request->url;
}

void redhttp_request_set_path(redhttp_request_t * request, const char *path)
{
  assert(request != NULL);

  if (request->path)
    free(request->path);

  request->path = redhttp_strdup(path);
}

const char *redhttp_request_get_path(redhttp_request_t * request)
{
  return request->path;
}

void redhttp_request_set_version(redhttp_request_t * request, const char *version)
{
  assert(request != NULL);

  if (request->version)
    free(request->version);

  request->version = redhttp_strdup(version);
}

const char *redhttp_request_get_version(redhttp_request_t * request)
{
  return request->version;
}

void redhttp_request_set_query_string(redhttp_request_t * request, const char *query_string)
{
  assert(request != NULL);

  if (request->query_string)
    free(request->query_string);

  request->query_string = redhttp_strdup(query_string);
}

const char *redhttp_request_get_query_string(redhttp_request_t * request)
{
  return request->query_string;
}

const char *redhttp_request_get_remote_addr(redhttp_request_t * request)
{
  return request->remote_addr;
}

const char *redhttp_request_get_remote_port(redhttp_request_t * request)
{
  return request->remote_port;
}

const char *redhttp_request_get_server_name(redhttp_request_t * request)
{
  const char* host = redhttp_request_get_header(request, "Host");

  // FIXME: make default hostname configurable
  if (host) {
    return host;
  } else {
    return request->server_addr;
  }
}

const char *redhttp_request_get_server_addr(redhttp_request_t * request)
{
  return request->server_addr;
}

const char *redhttp_request_get_server_port(redhttp_request_t * request)
{
  return request->server_port;
}

void redhttp_request_set_socket(redhttp_request_t * request, FILE * socket)
{
  request->socket = socket;
}

FILE *redhttp_request_get_socket(redhttp_request_t * request)
{
  return request->socket;
}

char *redhttp_request_get_content_buffer(redhttp_request_t * request)
{
  return request->content_buffer;
}

size_t redhttp_request_get_content_length(redhttp_request_t * request)
{
  return request->content_length;
}

int redhttp_request_read_status_line(redhttp_request_t * request)
{
  char *line, *ptr;
  char *method = NULL;
  char *url = NULL;
  char *version = NULL;

  assert(request != NULL);

  line = redhttp_request_read_line(request);
  if (line == NULL || strlen(line) == 0) {
    // FAIL!
    if (line)
      free(line);
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
  if (ptr > url && (strncmp("HTTP/", &ptr[1], 5) == 0 || strncmp("http/", &ptr[1], 5) == 0)) {
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

  free(line);

  // Success
  return 0;
}


int redhttp_request_read(redhttp_request_t * request)
{
  int result = redhttp_request_read_status_line(request);
  if (result)
    return result;

  if (strncmp(request->version, "0.9", 3)) {
    // Read in the headers
    while (!feof(request->socket)) {
      char *line = redhttp_request_read_line(request);
      if (line == NULL || strlen(line) < 1) {
        if (line)
          free(line);
        break;
      }
      redhttp_headers_parse_line(&request->headers, line);
      free(line);
    }

    // Read in PUT/POST content
    if (strncmp(request->method, "POST", 4) == 0) {
      const char *content_type = redhttp_headers_get(&request->headers, "Content-Type");
      const char *content_length = redhttp_headers_get(&request->headers, "Content-Length");
      int bytes_read = 0;

      if (content_type == NULL || content_length == NULL) {
        return REDHTTP_BAD_REQUEST;
      } else if (strncmp(content_type, "application/x-www-form-urlencoded", 33) == 0) {
        request->content_length = atoi(content_length);
        // FIXME: set maximum POST size
        request->content_buffer = calloc(1, request->content_length + 1);
        if (request->content_buffer) {
          bytes_read = fread(request->content_buffer, 1, request->content_length, request->socket);
          if (bytes_read != request->content_length) {
            perror("failed to read request");
            // FIXME: better response?
            return REDHTTP_BAD_REQUEST;
          } else {
            redhttp_request_parse_arguments(request, request->content_buffer);
          }
        }
      }
    }
  }
  // Success
  return 0;
}

void redhttp_request_free(redhttp_request_t * request)
{
  assert(request != NULL);

  if (request->method)
    free(request->method);
  if (request->url)
    free(request->url);
  if (request->version)
    free(request->version);
  if (request->path)
    free(request->path);
  if (request->path_glob)
    free(request->path_glob);
  if (request->query_string)
    free(request->query_string);
  if (request->content_buffer)
    free(request->content_buffer);

  if (request->socket)
    fclose(request->socket);

  redhttp_headers_free(&request->headers);
  redhttp_headers_free(&request->arguments);

  free(request);
}
